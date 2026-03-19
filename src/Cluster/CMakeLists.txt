file (GLOB cluster_source *.cpp)
file (GLOB cluster_header *.hpp)

add_library(Cluster ${cluster_source})



set_target_properties(Cluster PROPERTIES
        VERSION ${PROJECT_VERSION}
        SOVERSION 1
        PUBLIC_HEADER "${cluster_header}")
