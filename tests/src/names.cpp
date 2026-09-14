#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "uzuki2/parse_hdf5.hpp"

#include "test_subclass.h"
#include "utils.h"

TEST(Hdf5Names, Vector) {
    auto path = "TEST-vector.h5";

    // No names.
    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "integer");
        write_numbers<int>(vhandle, "data", { 1, 2, 3, 4, 5 }, H5::PredType::NATIVE_INT32);
    }
    {
        auto parsed = load_hdf5(path, "blub");
        EXPECT_EQ(parsed->type(), uzuki2::INTEGER);
        auto stuff = static_cast<const DefaultIntegerVector*>(parsed.get());
        EXPECT_FALSE(stuff->base.has_names);
    }

    // Plus names.
    {
        H5::H5File handle(path, H5F_ACC_RDWR);
        auto ghandle = handle.openGroup("blub");
        write_strings(ghandle, "names", { "A", "B", "C", "D", "E" });
    }
    {
        auto parsed = load_hdf5(path, "blub");
        EXPECT_EQ(parsed->type(), uzuki2::INTEGER);
        auto stuff = static_cast<const DefaultIntegerVector*>(parsed.get());
        EXPECT_TRUE(stuff->base.has_names);
        EXPECT_EQ(stuff->base.names.front(), "A");
        EXPECT_EQ(stuff->base.names.back(), "E");
    }
}

TEST(Hdf5Names, Scalar) {
    auto path = "TEST-vector.h5";

    // No names.
    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "integer");
        write_number(vhandle, "data", 999, H5::PredType::NATIVE_INT32);
    }
    {
        auto parsed = load_hdf5(path, "blub");
        EXPECT_EQ(parsed->type(), uzuki2::INTEGER);
        auto stuff = static_cast<const DefaultIntegerVector*>(parsed.get());
        EXPECT_TRUE(stuff->base.scalar);
        EXPECT_FALSE(stuff->base.has_names);
    }

    // Plus names.
    {
        H5::H5File handle(path, H5F_ACC_RDWR);
        auto ghandle = handle.openGroup("blub");
        write_strings(ghandle, "names", { "A" });
    }
    {
        auto parsed = load_hdf5(path, "blub");
        EXPECT_EQ(parsed->type(), uzuki2::INTEGER);
        auto stuff = static_cast<const DefaultIntegerVector*>(parsed.get());
        EXPECT_TRUE(stuff->base.has_names);
        EXPECT_EQ(stuff->base.names.size(), 1);
        EXPECT_EQ(stuff->base.names.front(), "A");
    }
}

TEST(Hdf5Names, ChunkStream) {
    auto path = "TEST-integer.h5";

    // Simulate multiple chunks so that we test the while{} loop for streaming values.
    const std::size_t nlen = 25000;
    std::vector<int> collected(nlen);
    std::vector<std::string> all_names(nlen);
    for (std::size_t i = 0; i < nlen; ++i) {
        collected[i] = i;
        all_names[i] = std::to_string(i);
    }

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "integer");
        write_numbers(vhandle, "data", collected, H5::PredType::NATIVE_INT32, /* chunk_size = */ 82);
        write_strings(vhandle, "names", all_names, /* variable = */ false, /* chunk_size = */ 119);
    }

    auto parsed = load_hdf5(path, "blub");
    EXPECT_EQ(parsed->type(), uzuki2::INTEGER);
    auto iptr = static_cast<const DefaultIntegerVector*>(parsed.get());
    EXPECT_EQ(iptr->base.values, collected);
    EXPECT_TRUE(iptr->base.has_names);
    EXPECT_EQ(iptr->base.names, all_names);
}

TEST(Hdf5Names, Error) {
    auto path = "TEST-vector.h5";

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "integer");
        write_numbers<int>(vhandle, "data", { 1, 2, 3, 4, 5 }, H5::PredType::NATIVE_INT32);
        write_numbers<int>(vhandle, "names", { 1, 2, 3, 4, 5 }, H5::PredType::NATIVE_UINT8);
    }
    expect_hdf5_error(path, "blub", "UTF-8 string");

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "integer");
        write_numbers<int>(vhandle, "data", { 1, 2, 3, 4, 5 }, H5::PredType::NATIVE_INT32);
        write_string(vhandle, "names", "A");
    }
    expect_hdf5_error(path, "blub", "1-dimensional dataset");

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "integer");
        write_numbers<int>(vhandle, "data", { 1, 2, 3, 4, 5 }, H5::PredType::NATIVE_INT32);
        write_strings(vhandle, "names", { "A", "B", "C", "D" });
    }
    expect_hdf5_error(path, "blub", "should be equal to the object length");
}
