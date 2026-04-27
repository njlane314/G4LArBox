#include "ElectronicsResponse.hh"

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

namespace G4LArBox
{
    ElectronicsResponse::ElectronicsResponse()
        : ElectronicsResponse(Config())
    {}

    ElectronicsResponse::ElectronicsResponse(const Config& config)
        : config_(config),
          rng_(config.seed),
          uniform_(0.0, 1.0)
    {
        GenerateSinglePEWaveform();
    }

    ElectronicsResponse::~ElectronicsResponse()
    {}

    std::vector<ElectronicsResponse::Waveform>
    ElectronicsResponse::GenerateWaveforms(const std::vector<OpticalHit>& hits)
    {
        int channel_count = config_.channel_count;
        for (const auto& hit : hits)
        {
            if (hit.channel >= channel_count)
            {
                channel_count = hit.channel + 1;
            }
        }

        const int sample_count = static_cast<int>(
            std::max(0.0, (config_.time_end_us - config_.time_begin_us) *
                              config_.sample_frequency_mhz));
        if (channel_count <= 0 || sample_count <= 0)
        {
            return {};
        }

        std::vector<std::vector<double>> raw_waveforms(
            channel_count, std::vector<double>(sample_count, 0.0));
        std::vector<bool> active_channels(channel_count, false);

        for (const auto& hit : hits)
        {
            if (hit.channel < 0 || hit.channel >= channel_count)
            {
                continue;
            }
            if (uniform_(rng_) > PhotonDetectionEfficiency(hit.wavelength_nm))
            {
                continue;
            }

            const int time_slice = TimeSlice(hit.time_ns);
            if (time_slice < 0 || time_slice >= sample_count)
            {
                continue;
            }

            AddWaveform(time_slice, raw_waveforms[hit.channel], SampleGain());
            active_channels[hit.channel] = true;
        }

        if (config_.dark_rate_hz > 0.0)
        {
            const double window_s = (config_.time_end_us - config_.time_begin_us) * 1.0e-6;
            std::poisson_distribution<int> dark_count(config_.dark_rate_hz * window_s);
            std::uniform_real_distribution<double> dark_time(config_.time_begin_us * 1000.0,
                                                             config_.time_end_us * 1000.0);
            for (int channel = 0; channel < channel_count; ++channel)
            {
                const int pulses = dark_count(rng_);
                for (int pulse = 0; pulse < pulses; ++pulse)
                {
                    const int time_slice = TimeSlice(dark_time(rng_));
                    if (time_slice < 0 || time_slice >= sample_count)
                    {
                        continue;
                    }
                    AddWaveform(time_slice, raw_waveforms[channel], SampleGain());
                    active_channels[channel] = true;
                }
            }
        }

        const double sample_period_s = 1.0e-6 / config_.sample_frequency_mhz;
        const double waveform_duration_s = sample_count * sample_period_s;
        std::poisson_distribution<int> pedestal_fluctuations(
            config_.pedestal_fluctuation_rate_hz * waveform_duration_s);
        std::uniform_int_distribution<int> sample_pick(0, sample_count - 1);
        std::bernoulli_distribution sign_pick(0.5);

        std::normal_distribution<double> sample_noise(0.0, config_.adc_sample_noise_sigma);

        std::vector<Waveform> output;
        for (int channel = 0; channel < channel_count; ++channel)
        {
            if (!active_channels[channel] && !config_.store_noise_only_channels)
            {
                continue;
            }

            const double pedestal = SamplePedestal();
            Waveform waveform;
            waveform.channel = channel;
            waveform.adc.reserve(sample_count);
            for (double sample : raw_waveforms[channel])
            {
                double noisy_sample = pedestal + sample;
                if (config_.adc_sample_noise_sigma > 0.0)
                {
                    noisy_sample += sample_noise(rng_);
                }
                waveform.adc.push_back(Digitize(noisy_sample));
            }

            const int fluctuations = pedestal_fluctuations(rng_);
            for (int fluctuation = 0; fluctuation < fluctuations; ++fluctuation)
            {
                const int sample = sample_pick(rng_);
                int value = waveform.adc[sample];
                value += sign_pick(rng_) ? config_.pedestal_fluctuation_amplitude_adc
                                         : -config_.pedestal_fluctuation_amplitude_adc;
                value = std::min(value, config_.saturation_adc);
                value = std::max(value, static_cast<int>(std::numeric_limits<short>::min()));
                waveform.adc[sample] = static_cast<short>(value);
            }

            output.push_back(std::move(waveform));
        }

        return output;
    }

    void ElectronicsResponse::GenerateSinglePEWaveform()
    {
        const int sample_count = static_cast<int>(
            std::max(1.0, config_.waveform_length_us * config_.sample_frequency_mhz));
        const double sample_width_us = 1.0 / config_.sample_frequency_mhz;
        constexpr int integration_steps = 8;

        single_pe_waveform_.assign(sample_count, 0.0);
        double max_amplitude = 0.0;
        double charge = 0.0;
        for (int sample = 0; sample < sample_count; ++sample)
        {
            const double t0 = sample * sample_width_us;
            double integral = 0.0;
            for (int step = 0; step < integration_steps; ++step)
            {
                const double t = t0 + (step + 0.5) * sample_width_us / integration_steps;
                integral += PulseShape(t);
            }

            const double value = integral / integration_steps;
            single_pe_waveform_[sample] = value;
            max_amplitude = std::max(max_amplitude, value);
            charge += value;
        }

        const double norm = config_.waveform_charge_normalized ? charge : max_amplitude;
        if (norm <= 0.0)
        {
            return;
        }

        for (double& sample : single_pe_waveform_)
        {
            sample /= norm;
            if (!config_.waveform_charge_normalized && sample < 1.0e-4)
            {
                sample = 0.0;
            }
        }
    }

    double ElectronicsResponse::PulseShape(double time_us) const
    {
        if (time_us <= 0.0 || config_.waveform_time_constant_us <= 0.0)
        {
            return 0.0;
        }

        const double power = config_.waveform_power_factor;
        return std::pow(10.0, 22.0) * config_.voltage_amplitude_for_spe *
               std::pow(time_us, power) *
               std::exp(-time_us / config_.waveform_time_constant_us) /
               std::tgamma(power + 1.0);
    }

    double ElectronicsResponse::PhotonDetectionEfficiency(double) const
    {
        return std::clamp(config_.quantum_efficiency, 0.0, 1.0);
    }

    double ElectronicsResponse::SampleGain()
    {
        if (config_.gain_spread <= 0.0 || config_.gain_mean_adc <= 0.0)
        {
            return std::max(0.0, config_.gain_mean_adc);
        }

        std::normal_distribution<double> gain(config_.gain_mean_adc,
                                              config_.gain_spread * config_.gain_mean_adc);
        return std::max(0.0, gain(rng_));
    }

    double ElectronicsResponse::SamplePedestal()
    {
        if (config_.adc_baseline_spread <= 0.0)
        {
            return config_.adc_baseline;
        }

        std::normal_distribution<double> pedestal(config_.adc_baseline,
                                                  config_.adc_baseline_spread);
        return pedestal(rng_);
    }

    int ElectronicsResponse::TimeSlice(double time_ns) const
    {
        const double time_us = time_ns / 1000.0;
        return static_cast<int>((time_us - config_.time_begin_us) *
                                config_.sample_frequency_mhz);
    }

    void ElectronicsResponse::AddWaveform(int time_slice,
                                          std::vector<double>& waveform,
                                          double gain)
    {
        for (size_t i = 0; i < single_pe_waveform_.size(); ++i)
        {
            const int sample = time_slice + static_cast<int>(i);
            if (sample < 0 || sample >= static_cast<int>(waveform.size()))
            {
                continue;
            }
            waveform[sample] += single_pe_waveform_[i] * gain;
        }
    }

    short ElectronicsResponse::Digitize(double value)
    {
        int adc = static_cast<int>(value);
        const double fraction = value - adc;
        if (fraction > 0.0 && uniform_(rng_) < fraction)
        {
            ++adc;
        }

        adc = std::min(adc, config_.saturation_adc);
        adc = std::max(adc, static_cast<int>(std::numeric_limits<short>::min()));
        adc = std::min(adc, static_cast<int>(std::numeric_limits<short>::max()));
        return static_cast<short>(adc);
    }
}
