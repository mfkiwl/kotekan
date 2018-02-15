/*****************************************
@file
@brief Write out eigenvectors to file.
- writeEigenvec : public KotekanProcess
- evFile
*****************************************/

#ifndef WRITE_EIGENVEC
#define WRITE_EIGENVEC

#include <unistd.h>
#include "buffer.h"
#include "KotekanProcess.hpp"
#include "errors.h"
#include "util.h"
#include "visUtil.hpp"
#include <highfive/H5File.hpp>
#include <highfive/H5DataSet.hpp>

/**
 * @class evFile
 * @brief Rolling HDF5 file for holding a set number of visibility eigenvectors on disk.
 *
 * Creates an HDF5 file in single writer multiple reader mode to serve as a ring buffer
 * for holding a specified number of eigenvectors, eigenvalues, and RMS values that
 * pass through the buffers along with the visbilities. These can be read from disk as
 * they are written out.
 *
 * @author  Tristan Pinsonneault-Marotte
 *
 */
class evFile {

public:
    /**
     * @brief Create SWMR HDF5 file
     *
     * @param fname Path to the file to write
     * @param num_eigenvectors The number of eigenvectors per frame
     * @param num_times The total length of the file (number of frames to buffer)
     * @param freqs The list of frequencies for which eigenvectors will be provided
     * @param inputs The list of inputs that make up the eigenvectors
     */
    evFile(const std::string & fname,
           const uint16_t & num_eigenvectors,
           const size_t & num_times,
           const std::vector<freq_ctype> & freqs,
           const std::vector<input_ctype> & inputs);
    ~evFile();

    /// Flush the HDF5 file to disk
    void flush();

    /**
     * @brief Write a set of eigenvectors/values to file for a given time and frequency
     *
     * @param new_time The time of the new sample
     * @param freq_ind The index to the list of frequencies corresponding to this sample
     * @param eigenvectors The eigenvectors to write out
     * @param eigenvalues The eigenvalues to write out
     * @param rms The RMS value to write out
     */
    void write_eigenvectors(time_ctype new_time, uint32_t freq_ind,
                            std::vector<cfloat> eigenvectors,
                            std::vector<float> eigenvalues, float rms);

    /// Access eigenvector dataset
    HighFive::DataSet evec();
    /// Access eigenvalue dataset
    HighFive::DataSet eval();
    /// Access RMS dataset
    HighFive::DataSet rms();
    /// Access times dataset
    HighFive::DataSet time();
    /// Access frequencies dataset
    HighFive::DataSet freq();
    /// Access inputs dataset
    HighFive::DataSet input();

private:

    // file dataset dimensions
    size_t ntimes;
    size_t ninput;
    size_t nfreq;
    size_t nev;
    // current timestamps held in file
    std::vector<uint64_t> curr_times;
    // position of the 'end' of the ring buffer
    size_t eof_ind;

    std::unique_ptr<HighFive::File> file;

};

/**
 * @class writeEigenvec
 * @brief Consumer ``KotekanProcess`` that extracts eigenvectors etc from a ``visBuffer``
 *        and writes them to disk in a rolling HDF5 file.
 *
 * This process reads the eigenvectors, eigenvalues, and RMS values carried in an input
 * visbility buffer and writes them to an ``evFile'' HDF5 file in single writer multiple
 * reader mode. This file has a specified length and rolls over when it fills up. 
 *
 * @par Buffers
 * @buffer in_buf The kotekan buffer from which the data is read.
 *     @buffer_format visBuffer structured
 *     @buffer_metadata visMetadata
 *
 * @conf  num_eigenvectors  Int. The number of eigenvectors carried with the visibilitues.
 * @conf  freq_ids          List of int. The frequency IDs to expect.
 * @conf  ev_file           String. The path to the file to write to (will be overwritten).
 * @conf  ev_file_len       Int. The number of frames to buffer in the file.
 *
 * @warning Any existing file with the provided filename will be clobbered.
 *
 * @author  Tristan Pinsonneault-Marotte
 *
 */
class writeEigenvec : public KotekanProcess {

public:
    /// Constructor. Loads config options. Creates output file.
    writeEigenvec(Config &config,
                  const string& unique_name,
                  bufferContainer &buffer_container);

    /// Destructor. Flushes file contents to disk.
    ~writeEigenvec();

    /// Not yet implemented, should update runtime parameters.
    void apply_config(uint64_t fpga_seq);

    /// Primary loop
    void main_thread();

private:
    /// Number of eigenvectors that will be provided
    size_t num_eigenvectors;
    /// File path to write to
    std::string ev_fname;
    /// Number of frames to hold in file
    size_t ev_file_len;

    /// Vectors of frequencies and inputs in the data
    std::vector<freq_ctype> freqs;
    std::vector<uint16_t> freq_ids;
    std::vector<input_ctype> inputs;

    /// File to write to
    std::unique_ptr<evFile> file;

    /// Input buffer
    Buffer * in_buf;

};


#endif
