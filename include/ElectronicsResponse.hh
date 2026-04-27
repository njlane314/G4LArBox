#ifndef ELECTRONICSRESPONSE_HH
#define ELECTRONICSRESPONSE_HH

#include <random>
#include <vector>

namespace G4LArBox
{
    class ElectronicsResponse
    {
    public:
        struct Config
        {
            double sample_frequency_mhz = 64.0;
            double time_begin_us = 0.0;
            double time_end_us = 10.0;
            double quantum_efficiency = 1.0;
            double dark_rate_hz = 0.0;
            double gain_mean_adc = 20.0;
            double gain_spread = 0.05;
            double adc_baseline = 2048.0;
            double adc_baseline_spread = 3.4;
            double adc_sample_noise_sigma = 1.5;
            int saturation_adc = 4095;
            double pedestal_fluctuation_rate_hz = 0.0;
            int pedestal_fluctuation_amplitude_adc = 1;
            bool store_noise_only_channels = false;
            double waveform_length_us = 2.0;
            double waveform_power_factor = 10.0;
            double waveform_time_constant_us = 0.006;
            double voltage_amplitude_for_spe = 0.04;
            bool waveform_charge_normalized = false;
            int channel_count = 0;
            unsigned int seed = 314159;
        };

        struct OpticalHit
        {
            int channel = -1;
            double time_ns = 0.0;
            double wavelength_nm = 0.0;
        };

        struct Waveform
        {
            int channel = -1;
            std::vector<short> adc;
        };

        ElectronicsResponse();
        explicit ElectronicsResponse(const Config& config);
        ~ElectronicsResponse();

        std::vector<Waveform> GenerateWaveforms(const std::vector<OpticalHit>& hits);

        double SampleFrequencyMHz() const { return config_.sample_frequency_mhz; }
        double TimeBeginUs() const { return config_.time_begin_us; }
        double TimeEndUs() const { return config_.time_end_us; }

    private:
        Config config_;
        std::vector<double> single_pe_waveform_;
        std::mt19937 rng_;
        std::uniform_real_distribution<double> uniform_;

        void GenerateSinglePEWaveform();
        double PulseShape(double time_us) const;
        double PhotonDetectionEfficiency(double wavelength_nm) const;
        double SampleGain();
        double SamplePedestal();
        int TimeSlice(double time_ns) const;
        void AddWaveform(int time_slice, std::vector<double>& waveform, double gain);
        short Digitize(double value);
    };
}

#endif // ELECTRONICSRESPONSE_HH
