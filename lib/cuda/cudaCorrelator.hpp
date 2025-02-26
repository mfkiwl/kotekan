#ifndef KOTEKAN_CUDA_CORRELATOR_CUH
#define KOTEKAN_CUDA_CORRELATOR_CUH

#include "cudaCommand.hpp"
#include "cudaDeviceInterface.hpp"
#include "n2k.hpp"

/**
 * @class cudaCorrelator
 * @brief cudaCommand for doing an N2 correlation, with Cuda code from Kendrick.
 *
 * @author Andre Renard (by kindly telling Dustin Lang what to type)
 *
 * An example of this stage being used can be found in
 * `config/tests/verify_cuda_n2k.yaml`.
 *
 * A CPU implementation is in `lib/testing/gpuSimulateN2k.hpp`.
 *
 * @par GPU Memory
 * @gpu_mem  gpu_mem_voltage  Input complex voltages of size samples_per_data_set * num_elements *
 * num_local_freq
 *   @gpu_mem_type   staging
 *   @gpu_mem_format Array of @c 4+4-bit complex
 * @gpu_mem  gpu_mem_correlation_triangle  Output complex correlation values of size per frame:
 * (samples_per_data_set / sub_integration_ntimes) * num_elements^2 * 2 * sizeof(int32)
 *   @gpu_mem_type   staging
 *   @gpu_mem_format Array of @c int32
 *
 * @conf   num_elements          Int.  Number of feeds.
 * @conf   num_local_freq        Int.  Number of frequencies.
 * @conf   samples_per_data_set  Int.  Number of time samples per Kotekan block.
 * @conf   sub_integration_ntime Int.  Number of time samples that will be summed into the
 * correlation matrix.
 *
 * Note: While the output is only supposed to fill the upper triangle
 * of the correlation matrices, this implementation fills a few of the
 * below-the-diagonal elements with non-zero values.
 */
class cudaCorrelator : public cudaCommand {
public:
    cudaCorrelator(kotekan::Config& config, const std::string& unique_name,
                   kotekan::bufferContainer& host_buffers, cudaDeviceInterface& device);
    ~cudaCorrelator();
    cudaEvent_t execute(int gpu_frame_id, const std::vector<cudaEvent_t>& pre_events) override;

protected:
private:
    // Common configuration values (which do not change in a run)
    /// Number of elements on the telescope (e.g. 2048 - CHIME, 256 - Pathfinder).
    int32_t _num_elements;
    /// Number of frequencies per data stream sent to each node.
    int32_t _num_local_freq;
    /// Total samples in each dataset. Must be a value that is a power of 2.
    int32_t _samples_per_data_set;
    // Number of time samples into each of the output correlation
    // triangles.  The number of output correlation triangles is the
    // length of the input frame divided by this value.
    int32_t _sub_integration_ntime;

    /// GPU side memory name for the voltage input
    std::string _gpu_mem_voltage;
    /// GPU side memory name for correlator output
    std::string _gpu_mem_correlation_triangle;

    // Cuda kernel wrapper object
    n2k::Correlator n2correlator;
};

#endif // KOTEKAN_CUDA_CORRELATOR_CUH
