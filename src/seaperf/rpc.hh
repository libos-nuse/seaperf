#pragma once

#include <rpc/rpc.hh>

namespace seaperf {

enum { RPC_TCPBENCH };

struct BenchmarkResult {
  uint64_t duration_sec;
  uint64_t byte_cnt;
};

struct serializer {};

template <typename T, typename Input>
inline T read_integral(Input& in) {
  static_assert(std::is_integral<T>::value, "Only integral types are allowed");
  T v;
  in.read(reinterpret_cast<char*>(&v), sizeof(T));
  return le_to_cpu(v);
}

template <typename T, typename Output>
inline void write_integral(Output& out, T v) {
  static_assert(std::is_integral<T>::value, "Only integral types are allowed");
  v = cpu_to_le(v);
  out.write(reinterpret_cast<const char*>(&v), sizeof(T));
}

template <typename Input>
inline int32_t read(serializer, Input& in, rpc::type<int32_t>) {
  return read_integral<int32_t>(in);
}
template <typename Input>
inline uint32_t read(serializer, Input& in, rpc::type<uint32_t>) {
  return read_integral<uint32_t>(in);
}
template <typename Input>
inline int64_t read(serializer, Input& in, rpc::type<int64_t>) {
  return read_integral<int64_t>(in);
}
template <typename Input>
inline uint64_t read(serializer, Input& in, rpc::type<uint64_t>) {
  return read_integral<uint64_t>(in);
}
template <typename Output>
inline void write(serializer, Output& out, int32_t v) {
  return write_integral(out, v);
}
template <typename Output>
inline void write(serializer, Output& out, uint32_t v) {
  return write_integral(out, v);
}
template <typename Output>
inline void write(serializer, Output& out, int64_t v) {
  return write_integral(out, v);
}
template <typename Output>
inline void write(serializer, Output& out, uint64_t v) {
  return write_integral(out, v);
}

template <typename Input>
inline BenchmarkResult read(serializer s, Input& in,
                            rpc::type<BenchmarkResult>) {
  BenchmarkResult res;
  res.duration_sec = read(s, in, rpc::type<uint64_t>());
  res.byte_cnt = read(s, in, rpc::type<uint64_t>());
  return res;
}
template <typename Output>
inline void write(serializer s, Output& out, BenchmarkResult v) {
  write(s, out, v.duration_sec);
  write(s, out, v.byte_cnt);
}
}  // namespace seaperf
