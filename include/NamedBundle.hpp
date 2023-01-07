#pragma once

#include <utility>
#include <fstream>

#include <iostream>

#include <manif/manif.h>

#include <nlohmann/json.hpp>
#include <Json2Manif.hpp>

#include "enum.h"

namespace named_bundle {

template<typename S, typename T>
void print_element(T t, std::size_t i) {
  std::cout << S::_from_index(i)._to_string() << ": " << t << " ";
}

template<typename S, typename T, std::size_t... I>
void print(const T &state, std::index_sequence<I...>) {
  (print_element<S>(state.template element<I>(), I), ...);
}

template<typename S, typename T>
void print(const T &state) {
  print<S>(state, std::make_index_sequence<S::_size()>{});
}

// TODO, make it consistend with other to_json conversions and move to Manif2Json.
template<typename S, typename T>
auto to_json(const T &state) {
  nlohmann::json j;
  j["type"] = "Bundle";
  j["elements"] = nlohmann::json::array();

  auto fill = [](nlohmann::json &j, std::size_t i, auto el) {
    j["elements"][i] = el;
    j["elements"][i]["name"] = S::_from_index(i)._to_string();
  };

  auto lam = [&]<std::size_t... I>(std::index_sequence<I...>) {
    (fill(j, I, state.template element<I>()), ...);
  };

  lam(std::make_index_sequence<manif::internal::traits<T>::BundleSize>{});

  assert(!j["type"].template get<std::string>().empty());

  return j;
}

//template<typename S, typename T>
//auto from_json(T &state) {
//  nlohmann::json j;
//  j["type"] = "Bundle";
//  j["elements"] = nlohmann::json::array();
//
//  auto fill = [](nlohmann::json &j, std::size_t i, auto el) {
//    j["elements"][i] = el;
//    j["elements"][i]["name"] = S::_from_index(i)._to_string();
//  };
//
//  auto lam = [&]<std::size_t... I>(std::index_sequence<I...>) {
//    (fill(j, I, state.template element<I>()), ...);
//  };
//
//  lam(std::make_index_sequence<manif::internal::traits<T>::BundleSize>{});
//
//  assert(!j["type"].template get<std::string>().empty());
//
//  return j;
//}

template<typename T>
constexpr auto size() {
  return manif::internal::traits<T>::BundleSize;
}

}

#include <doctest.h>

BETTER_ENUM(StateNames, std::size_t,
            Attitude,
            AngularVelocity,
            Position,
            LinearVelocity,
            Swing
)

TEST_SUITE("manif named") {

TEST_CASE("named bundle") {

  using State = manif::Bundle<
      double,
      manif::SO3,
      manif::R3,
      manif::R3,
      manif::R3,
      manif::SO2
  >;

  static_assert(StateNames::_size() == manif::internal::traits<State>::BundleSize);

  using at = StateNames;

  State state = State::Random();

  SUBCASE("print") {
    named_bundle::print<StateNames>(state);
  }

  SUBCASE("bundle state to_json") {
    State state = State::Identity();

    state.element<at::AngularVelocity>() = manif::R3d(Eigen::Vector3d(1, 2, 3));
//  bundle::print<StateNames>(state);
    nlohmann::json json;
    json["state"] = named_bundle::to_json<StateNames>(state);

    std::cout << json.dump() << "\n";

    CHECK(json["state"]["elements"][1]["name"].get<std::string>() == "AngularVelocity");
    CHECK(json["state"]["elements"][1]["coeffs"][0].get<double>() == 1.0);
    CHECK(json["state"]["elements"][1]["coeffs"][1].get<double>() == 2.0);
    CHECK(json["state"]["elements"][1]["coeffs"][2].get<double>() == 3.0);

    std::string dump_file_path = "/home/slovak/kalman-benchmark/test/manifolds/to_json.msg";
    std::remove(dump_file_path.c_str());

    const std::vector<std::uint8_t> msgpack = nlohmann::json::to_msgpack(json);
    std::ofstream(dump_file_path, std::ios::app | std::ios::binary).write(
        reinterpret_cast<const char *>(msgpack.data()), msgpack.size() * sizeof(uint8_t));

    nlohmann::json json_new = nlohmann::json::from_msgpack(std::ifstream(dump_file_path, std::ios::binary));
    std::cout << json_new.dump() << "\n";
    CHECK(json_new == json);

    State state_new;

//    state_new = from_json<StateNames>(json_new); // TODO, implement.
  }

}

}
