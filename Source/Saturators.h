#pragma once

#include <JuceHeader.h>
#include "PluginParameters.h"

#define TARGET_SAMPLING_RATE 192000.0
class FoldbackSaturator
{
public:
    FoldbackSaturator(double defaultDrive = Parameters::defaultDrive,
        float defaultThresh = 0.8f,
        double defaultStereoWidth = Parameters::defaultStereoWidth)
        : drive(defaultDrive),
        threshold(defaultThresh),
        stereoWidth(defaultStereoWidth)
    {
        drive.setCurrentAndTargetValue(defaultDrive);
        stereoWidth.setCurrentAndTargetValue(defaultStereoWidth);
    }

    void prepareToPlay(double sampleRate)
    {
        drive.reset(sampleRate, 0.03); // Smoothing attack/release veloce
        stereoWidth.reset(sampleRate, 0.03);
    }

    void setDrive(double value)
    {
        drive.setTargetValue(value);
    }
    void setThreshold(float thresh)
    {
        threshold = thresh;
    }
    void setStereoWidth(float width)
    {
        stereoWidth.setTargetValue(width);
    }
    // Procesing: apply foldback per canale with drive and threshold
    void processBlock(juce::AudioBuffer<float>& buffer, const juce::AudioBuffer<double>& modulatedDriveBuffer)
    {
        const int numChannels = buffer.getNumChannels();
        const int numSamples = buffer.getNumSamples();

        auto bufferData = buffer.getArrayOfWritePointers();
        auto modDriveData = modulatedDriveBuffer.getArrayOfReadPointers();

        for (int sample = 0; sample < numSamples; ++sample)
        {
            
            double modulatedDrive = modDriveData[0][sample];
            float currentWidth = stereoWidth.getNextValue();

            // Calcola i bias per L e R
            float biasL = currentWidth * (-0.5f);
            float biasR = currentWidth * (+0.5f);

            // Processa ogni canale
            for (int ch = 0; ch < numChannels; ++ch)
            {
                float bias = (ch == 0) ? biasL : biasR;

                // Applica: drive modulato + bias, poi foldback
                float driven = bufferData[ch][sample] * modulatedDrive + bias;
                bufferData[ch][sample] = foldback(driven, threshold);
            }
        }
    }

    static float foldback(float x, float thresh)
    {
		// 1. Controllo della soglia -> Se il segnale supera la soglia applica la distorsione foldback, altrimenti ritorna il segnale inalterato
        if (std::abs(x) > thresh)
            // 2. Applicazione della distorsione foldback
            return std::abs(std::fmod(x - thresh, thresh * 4)) - thresh;
		// 3. return del segnale inalterato
        return x;
    }

private:
    SmoothedValue<double, ValueSmoothingTypes::Linear> drive;
    SmoothedValue<double, ValueSmoothingTypes::Linear> stereoWidth;
    float threshold;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FoldbackSaturator);
};

class SineFoldSaturator
{
public:
    SineFoldSaturator(double defaultDrive = Parameters::defaultDrive,
        double defaultStereoWidth = Parameters::defaultStereoWidth,
        bool defaultOversampling = Parameters::defaultOversampling)
        : drive(defaultDrive),
        stereoWidth(defaultStereoWidth),
        oversampling(defaultOversampling)
    {
        drive.setCurrentAndTargetValue(defaultDrive);
        stereoWidth.setCurrentAndTargetValue(defaultStereoWidth);
    }

    void prepareToPlay(double sampleRate, int samplesPerBlock, int numCh)
    {
        drive.reset(sampleRate, 0.03); // Smoothing attack/release veloce
        stereoWidth.reset(sampleRate, 0.03);
        // Imposta DC blocker (highpass 5Hz, come hip~ 5 in PD)
        auto coeffs = juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, 10.0);
        for (int ch = 0; ch < 2; ++ch)
        {
            dcBlocker[ch].coefficients = coeffs;
            dcBlocker[ch].reset();
        }

        resetOversampler();
        
    }
    void setOversampling(bool shouldOversample)
    {
        if (oversampling == shouldOversample)
            return; // Nessun cambiamento
		oversampling = shouldOversample;
        needsOversamplerReset = true; // Flag per resettare al prossimo processBlock
    }
    void setDrive(double value)
    {
        drive.setTargetValue(value);
    }
    void setStereoWidth(float width)
    {
        stereoWidth.setTargetValue(width);
    }
    // Procesing: apply foldback per canale wsddith drive and threshold
    void processBlock(juce::AudioBuffer<float>& buffer, const juce::AudioBuffer<double>& envelopeBuffer)
    {
		if (needsOversamplerReset)//check per resettare l'oversampler se necessario, lo facciamo ora per evitare di sentire click
        {
            resetOversampler();
            needsOversamplerReset = false;
        }
        const int numChannels = buffer.getNumChannels();
        const int numSamples = buffer.getNumSamples();
		auto driveValue = 1.0f;
        auto bufferData = buffer.getArrayOfWritePointers();
        auto envData = envelopeBuffer.getReadPointer(0);
		//OVERSAMPLING
        juce::dsp::AudioBlock<float> block(buffer.getArrayOfWritePointers(),
            numChannels,
            0,
            numSamples);
        auto oversampledBlock = oversampler->processSamplesUp(block);
        const size_t numOversampledChannels = oversampledBlock.getNumChannels();
        const size_t numOversampledSamples = oversampledBlock.getNumSamples();
        
        for (size_t sample = 0; sample < numOversampledSamples; ++sample)
        {
            size_t nativeIndex = sample / oversamplingFactor;            // Calcola l'indice del campione corrispondente nel buffer a frequenza nativa.
            if (nativeIndex >= envelopeBuffer.getNumSamples())           // Controlla se l'indice nativo calcolato supera i limiti del buffer dell'inviluppo.
                nativeIndex = envelopeBuffer.getNumSamples() - 1;        // Se fuori limite, imposta l'indice all'ultimo campione valido.

            float env = envelopeBuffer.getSample(0, nativeIndex);        // Recupera il valore dell'inviluppo dal canale 0 all'indice nativo corretto。


            float currentWidth = stereoWidth.getNextValue();
			driveValue = drive.getNextValue();
            // Calcola i bias per L e R
            float biasL = currentWidth * (-0.5f);
            float biasR = currentWidth * (+0.5f);

            // Processa ogni canale
            for (size_t ch = 0; ch < numOversampledChannels; ++ch)
            {
                float bias = (ch == 0) ? biasL : biasR;

                // 1. input  drive
                float driven = bufferData[ch][sample] * driveValue;

                // 2. + bias
                driven += bias;

                // 3.  envelope_mod
                driven *= env;

                // 4. sin~ (o altra waveshaping function)
                bufferData[ch][sample] = sineFold(driven);


            }
        }
        oversampler->processSamplesDown(block);
        // ═══════════════════════════════════════════════════════════
        // DC BLOCKER (a sample rate NATIVO dopo il downsampling)
        // ═══════════════════════════════════════════════════════════
        for (int ch = 0; ch < numChannels; ++ch)
        {
            float* channelData = buffer.getWritePointer(ch);
            for (int i = 0; i < numSamples; ++i) {
                channelData[i] = dcBlocker[ch].processSample(channelData[i]);
                // 6. Gain compensation (*~ 0.5)
                bufferData[ch][i] *= 0.5f;
            }
        }
    }


    static float sineFold(float x)
    {
        // Modula il segnale con una funzione sinusoidale
        // drive controlla quante volte il segnale viene "piegato"
        return std::sin(juce::MathConstants<float>::twoPi * x );
    }
    int getLatencySamples() const noexcept
    {
        if (oversampler)
            return static_cast<int>(oversampler->getLatencyInSamples());
        return 0;
    }

private:
    void resetOversampler()
    {
        if (oversampling)
        {
            // Calcola il fattore di oversampling in base al sample rate target
            oversamplingFactor = static_cast<int>(TARGET_SAMPLING_RATE / (drive.getCurrentValue() > 0 ? drive.getCurrentValue() : 1.0));
            oversamplingFactor = juce::jmin(4, juce::jmax(1, oversamplingFactor)); // Limita il fattore tra 1 e 4
            oversampler = std::make_unique<dsp::Oversampling<float>>(
                2, // numero di canali
                static_cast<size_t>(std::log2(oversamplingFactor)), // fattore di oversampling come potenza di 2
                dsp::Oversampling<float>::FilterType::filterHalfBandPolyphaseIIR,
                true, // massima qualità
                true  // latenza intera
            );
        }
        else
        {
            oversamplingFactor = 1;
            oversampler.reset();
        }
    }
    SmoothedValue<double, ValueSmoothingTypes::Linear> drive;
    SmoothedValue<double, ValueSmoothingTypes::Linear> stereoWidth;
    juce::dsp::IIR::Filter<float> dcBlocker[2];
    bool oversampling;
    bool needsOversamplerReset = false;
    double overSampledRate = 0;
    std::unique_ptr<dsp::Oversampling<float>> oversampler;
    int oversamplingFactor = 1;
    int latency = 0;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SineFoldSaturator);
};















class TriFoldSaturator
{
public:
    TriFoldSaturator(double defaultDrive = Parameters::defaultDrive,
        float defaultThresh = 0.8f,
        double defaultStereoWidth = Parameters::defaultStereoWidth)
        : drive(defaultDrive),
        threshold(defaultThresh),
        stereoWidth(defaultStereoWidth)
    {
        drive.setCurrentAndTargetValue(defaultDrive);
        stereoWidth.setCurrentAndTargetValue(defaultStereoWidth);
    }

    void prepareToPlay(double sampleRate)
    {
        drive.reset(sampleRate, 0.03); // Smoothing attack/release veloce
        stereoWidth.reset(sampleRate, 0.03);
    }

    void setDrive(double value)
    {
        drive.setTargetValue(value);
    }
    void setThreshold(float thresh)
    {
        threshold = thresh;
    }
    void setStereoWidth(float width)
    {
        stereoWidth.setTargetValue(width);
    }
    // Procesing: apply foldback per canale with drive and threshold
    void processBlock(juce::AudioBuffer<float>& buffer, const juce::AudioBuffer<double>& modulatedDriveBuffer)
    {
        const int numChannels = buffer.getNumChannels();
        const int numSamples = buffer.getNumSamples();

        auto bufferData = buffer.getArrayOfWritePointers();
        auto modDriveData = modulatedDriveBuffer.getArrayOfReadPointers();

        for (int sample = 0; sample < numSamples; ++sample)
        {

            double modulatedDrive = modDriveData[0][sample];
            float currentWidth = stereoWidth.getNextValue();

            // Calcola i bias per L e R
            float biasL = currentWidth * (-0.5f);
            float biasR = currentWidth * (+0.5f);

            // Processa ogni canale
            for (int ch = 0; ch < numChannels; ++ch)
            {
                float bias = (ch == 0) ? biasL : biasR;

                // Applica: drive dulato + bias, poi foldback
                float driven = bufferData[ch][sample] * modulatedDrive + bias;
                bufferData[ch][sample] = triangleWavefolder(driven, threshold);
            }
        }
    }

    static float triangleWavefolder(float x, float drive)
    {
        // Normalizza con drive per controllare il folding
        float normalized = x * drive;

        // Triangle wave formula dalla documentazione Stanford
        float period = 1.0f / drive;
        float phase = normalized + period / 4.0f;

        return 4.0f * std::abs((phase / period) - std::floor((phase / period) + 0.5f)) - 1.0f;
    }

private:
    SmoothedValue<double, ValueSmoothingTypes::Linear> drive;
    SmoothedValue<double, ValueSmoothingTypes::Linear> stereoWidth;
    float threshold;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TriFoldSaturator);
};