# Project Dependencies

This document is automatically generated from `CMakeLists.txt` files for `arrp`.

## Dependency Diagram

```mermaid
graph TD
    arrp["arrp::arrp"]

    subgraph Core["Core Dependencies (via CPM)"]
        RUNONEND["RunOnEnd 1.4.5"]
    end

    subgraph Test["Test Dependencies (Optional)"]
        GTEST["gtest v1.17.0"]
        NLOHMANNJSON["nlohmann_json v3.12.0"]
    end

    arrp --> RUNONEND
    arrp -. "arrp_BUILD_TESTS=ON" .-> GTEST
    arrp -. "arrp_BUILD_TESTS=ON" .-> NLOHMANNJSON
```

## Dependency Breakdown

| Dependency | Repository / Target | Version | Type | Scope / Platform |
| :--- | :--- | :--- | :--- | :--- |
| **RunOnEnd** | [`siddiqsoft/RunOnEnd`](https://github.com/siddiqsoft/RunOnEnd) | 1.4.5 | `CPM` | All Platforms (`INTERFACE`) |
| **gtest** | [`google/googletest`](https://github.com/google/googletest) | v1.17.0 | `CPM` | Test Target Only (`arrp_BUILD_TESTS=ON`) |
| **nlohmann_json** | [`nlohmann/json`](https://github.com/nlohmann/json) | v3.12.0 | `CPM` | Test Target Only (`arrp_BUILD_TESTS=ON`) |
