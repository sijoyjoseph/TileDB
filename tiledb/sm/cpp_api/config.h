/**
 * @file   tiledb_cpp_api_config.h
 *
 * @author Ravi Gaddipati
 *
 * @section LICENSE
 *
 * The MIT License
 *
 * @copyright Copyright (c) 2017-2018 TileDB, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * @section DESCRIPTION
 *
 * This file declares the C++ API for the TileDB Config object.
 */

#ifndef TILEDB_CPP_API_CONFIG_H
#define TILEDB_CPP_API_CONFIG_H

#include "config_iter.h"
#include "tiledb.h"
#include "utils.h"

#include <memory>
#include <string>

namespace tiledb {

namespace impl {
struct ConfigProxy;
}

/**
 * Carries configuration parameters for a context.
 *
 * **Example:**
 *
 * @code{.cpp}
 * Config conf();
 * conf["vfs.s3.region"] = "us-east-1a";
 * conf["vfs.s3.use_virtual_addressing"] = "true";
 * Context ctx(conf);
 * // array/kv operations with ctx
 * @endcode
 * */
class TILEDB_EXPORT Config {
 public:
  using iterator = impl::ConfigIter;
  /* ********************************* */
  /*     CONSTRUCTORS & DESTRUCTORS    */
  /* ********************************* */

  Config();

  /**
   * Constructor that takes as input a filename (URI) that stores the config
   * parameters. The file must have the following (text) format:
   *
   * {parameter} {value}
   *
   * Anything following a `#` character is considered a comment and, thus, is
   * ignored.
   *
   * See `set` for the various TileDB config parameters and allowed values.
   *
   * @param filename The name of the file where the parameters will be read
   *     from.
   */
  explicit Config(const std::string& filename);

  /** Constructor from a C config object. */
  explicit Config(tiledb_config_t** config);

  /* ********************************* */
  /*                API                */
  /* ********************************* */

  /** Returns the pointer to the TileDB C config object. */
  std::shared_ptr<tiledb_config_t> ptr() const;

  /** Auxiliary operator for getting the underlying C TileDB object. */
  operator tiledb_config_t*() const;

  /**
   * Sets a config parameter-value pair.
   *
   * **Parameters**
   *
   * - `sm.tile_cache_size` <br>
   *    The tile cache size in bytes. Any `uint64_t` value is acceptable. <br>
   *    **Default**: 10,000,000
   * - `sm.array_schema_cache_size` <br>
   *    The array schema cache size in bytes. Any `uint64_t` value is
   *    acceptable. <br>
   *    **Default**: 10,000,000
   * - `sm.fragment_metadata_cache_size` <br>
   *    The fragment metadata cache size in bytes. Any `uint64_t` value is
   *    acceptable. <br>
   *    **Default**: 10,000,000
   * - `sm.number_of_threads` <br>
   *    The number of allocated threads per TileDB context. <br>
   *    **Default**: number of cores
   * - `vfs.max_parallel_ops` <br>
   *    The maximum number of VFS parallel operations. <br>
   *    **Default**: number of cores
   * - `vfs.min_parallel_size` <br>
   *    The minimum number of bytes in a parallel VFS operation. (Does not
   *    affect parallel S3 writes.) <br>
   *    **Default**: 10MB
   * - `vfs.s3.region` <br>
   *    The S3 region, if S3 is enabled. <br>
   *    **Default**: us-east-1
   * - `vfs.s3.scheme` <br>
   *    The S3 scheme (`http` or `https`), if S3 is enabled. <br>
   *    **Default**: https
   * - `vfs.s3.endpoint_override` <br>
   *    The S3 endpoint, if S3 is enabled. <br>
   *    **Default**: ""
   * - `vfs.s3.use_virtual_addressing` <br>
   *    The S3 use of virtual addressing (`true` or `false`), if S3 is
   *    enabled. <br>
   *    **Default**: true
   * - `vfs.s3.multipart_part_size` <br>
   *    The part size (in bytes) used in S3 multipart writes, if S3 is enabled.
   *    Any `uint64_t` value is acceptable. Note: `vfs.s3.multipart_part_size *
   *    vfs.max_parallel_ops` bytes will be buffered before issuing multipart
   *    uploads in parallel. <br>
   *    **Default**: 5*1024*1024
   * - `vfs.s3.connect_timeout_ms` <br>
   *    The connection timeout in ms. Any `long` value is acceptable. <br>
   *    **Default**: 3000
   * - `vfs.s3.connect_max_tries` <br>
   *    The maximum tries for a connection. Any `long` value is acceptable. <br>
   *    **Default**: 5
   * - `vfs.s3.connect_scale_factor` <br>
   *    The scale factor for exponential backofff when connecting to S3.
   *    Any `long` value is acceptable. <br>
   *    **Default**: 25   * - `vfs.s3.request_timeout_ms` <br>
   *    The request timeout in ms. Any `long` value is acceptable. <br>
   *    **Default**: 3000
   * - `vfs.hdfs.name_node"` <br>
   *    Name node for HDFS. <br>
   *    **Default**: ""
   * - `vfs.hdfs.username` <br>
   *    HDFS username. <br>
   *    **Default**: ""
   * - `vfs.hdfs.kerb_ticket_cache_path` <br>
   *    HDFS kerb ticket cache path. <br>
   *    **Default**: ""
   */
  Config& set(const std::string& param, const std::string& value);

  /**
   * Get a parameter from the configuration by key.
   * @param param Key
   * @return Value
   */
  std::string get(const std::string& param) const;

  /** Enables setting parameters with `[]`. */
  impl::ConfigProxy operator[](const std::string& param);

  /** Unsets a config parameter. */
  Config& unset(const std::string& param);

  /** Iterate over params starting with a prefix **/
  iterator begin(const std::string& prefix) {
    return iterator{*this, prefix, false};
  }

  /** Iterate over all params. **/
  iterator begin() {
    return iterator{*this, "", false};
  }

  /** End iterator **/
  iterator end() {
    return iterator{*this, "", true};
  }

  /* ********************************* */
  /*          STATIC FUNCTIONS         */
  /* ********************************* */

  /** Wrapper function for freeing a config C object. */
  static void free(tiledb_config_t* config);

 private:
  /* ********************************* */
  /*         PRIVATE ATTRIBUTES        */
  /* ********************************* */

  /** The TileDB C config object. */
  std::shared_ptr<tiledb_config_t> config_;

  /* ********************************* */
  /*          PRIVATE METHODS          */
  /* ********************************* */

  /** Creates the TileDB C config object. */
  void create_config();
};

namespace impl {

/** Proxy to set params via operator `[]`. */
struct TILEDB_EXPORT ConfigProxy {
  ConfigProxy(Config& conf, std::string param)
      : conf(conf)
      , param(std::move(param)) {
  }

  template <typename T>
  ConfigProxy& operator=(const T& val) {
    conf.set(param, std::to_string(val));
    return *this;
  }

  ConfigProxy& operator=(const char* val) {
    conf.set(param, std::string(val));
    return *this;
  }

  ConfigProxy& operator=(const std::string& val) {
    conf.set(param, val);
    return *this;
  }

  ConfigProxy operator[](const std::string& append) {
    return {conf, param + append};
  }

  operator std::string() {
    return conf.get(param);
  }

  Config& conf;
  const std::string param;
};

}  // namespace impl

}  // namespace tiledb

#endif  // TILEDB_CPP_API_CONFIG_H
