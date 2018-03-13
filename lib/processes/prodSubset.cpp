#include "prodSubset.hpp"
#include "visBuffer.hpp"

REGISTER_KOTEKAN_PROCESS(prodSubset);

prodSubset::prodSubset(Config &config,
                               const string& unique_name,
                               bufferContainer &buffer_container) :
    KotekanProcess(config, unique_name, buffer_container,
                   std::bind(&prodSubset::main_thread, this)) {

    // Fetch any simple configuration
    num_elements = config.get_int(unique_name, "num_elements");
    num_eigenvectors =  config.get_int(unique_name, "num_eigenvectors");
    //num_prod = config.get_int(unique_name, "num_prod");

    num_prod = num_elements * (num_elements + 1) / 2;

    // Get buffers
    in_buf = get_buffer("in_buf");
    register_consumer(in_buf, unique_name.c_str());
    // TODO: Size of buffer is not adjusted for baseline subset.
    //       ~ 3/4 of buffer will be unused.
    out_buf = get_buffer("out_buf");
    register_producer(out_buf, unique_name.c_str());

    // Type of product selection based on config parameter
    prod_subset_type = config.get_string(unique_name, "prod_subset_type");

    if (prod_subset_type == "autos") {
        int idx = 0;
        for (int ii=0; ii<num_elements; ii++) {
            for (int jj=ii; jj<num_elements; jj++) {
                // Only add auto-correlations
                if (jj==ii) {
                    prod_ind.push_back(idx);
                }
                idx++;
            }
        }
    } else if (prod_subset_type == "baseline") {
        // Define criteria for baseline selection based on config parameters
        xmax = config.get_int(unique_name, "max_ew_baseline");
        ymax = config.get_int(unique_name, "max_ns_baseline");
        // Find the products in the subset
        for (size_t ii = 0; ii < num_prod; ii++) {
            if (max_bl_condition(ii, num_elements, xmax, ymax)) {
                prod_ind.push_back(ii);
            }
        }
    } else if (prod_subset_type == "input_list") {
        input_list = config.get_int_array(unique_name, "input_list");
        // Find the products in the subset
        for (size_t ii = 0; ii < num_prod; ii++) {
            if (input_list_condition(ii, num_elements, input_list)) {
                prod_ind.push_back(ii);
            }
        }
    }
    subset_num_prod = prod_ind.size();
}

void prodSubset::apply_config(uint64_t fpga_seq) {

}

void prodSubset::main_thread() {

    unsigned int output_frame_id = 0;
    unsigned int input_frame_id = 0;

    while (!stop_thread) {

        // Wait for the input buffer to be filled with data
        if(wait_for_full_frame(in_buf, unique_name.c_str(),
                               input_frame_id) == nullptr) {
            break;
        }

        // Wait for the output buffer frame to be free
        if(wait_for_empty_frame(out_buf, unique_name.c_str(),
                                output_frame_id) == nullptr) {
            break;
        }

        // Get a view of the current frame
        auto input_frame = visFrameView(in_buf, input_frame_id);

        // Allocate metadata and get output frame
        allocate_new_metadata_object(out_buf, output_frame_id);
        // Create view to output frame
        auto output_frame = visFrameView(out_buf, output_frame_id,
                                     num_elements, subset_num_prod,
                                     num_eigenvectors);

        // Copy over subset of visibilities
        for (size_t i = 0; i < subset_num_prod; i++) {
            output_frame.vis[i] = input_frame.vis[prod_ind[i]];
            output_frame.weight[i] = input_frame.weight[prod_ind[i]];
        }

        // Copy the non-visibility parts of the buffer
        output_frame.copy_nonvis_buffer(input_frame);
        // Copy metadata
        output_frame.copy_nonconst_metadata(input_frame);

        // Mark the buffers and move on
        mark_frame_full(out_buf, unique_name.c_str(), output_frame_id);
        mark_frame_empty(in_buf, unique_name.c_str(), input_frame_id);

        // Advance the current frame id
        output_frame_id = (output_frame_id + 1) % out_buf->num_frames;
        input_frame_id = (input_frame_id + 1) % in_buf->num_frames;

    }
}
