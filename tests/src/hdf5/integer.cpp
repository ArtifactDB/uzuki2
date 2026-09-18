#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "uzuki2/parse_hdf5.hpp"

#include "utils.h"
#include "../test_subclass.h"

TEST(Hdf5Integer, Vector) {
    auto path = "TEST-integer.h5";
    std::vector<std::int32_t> data{ 1, 2, 3, 4, 5, 0, -1 };

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "integer");
        write_numbers(vhandle, "data", data, H5::PredType::NATIVE_INT32);
    }

    auto parsed = load_hdf5(path, "blub");
    EXPECT_EQ(parsed->type(), uzuki2::INTEGER);
    auto iptr = static_cast<const DefaultIntegerVector*>(parsed.get());
    EXPECT_EQ(iptr->base.values, data);
    EXPECT_FALSE(iptr->base.scalar);

    // Test coverage of the relevant Dummy class.
    validate_hdf5(path, "blub");
}

TEST(Hdf5Integer, Scalar) {
    auto path = "TEST-integer.h5";

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "integer");
        write_number(vhandle, "data", 999, H5::PredType::NATIVE_INT32);
    }

    auto parsed = load_hdf5(path, "blub");
    EXPECT_EQ(parsed->type(), uzuki2::INTEGER);
    auto iptr = static_cast<const DefaultIntegerVector*>(parsed.get());
    EXPECT_EQ(iptr->size(), 1);
    EXPECT_EQ(iptr->base.values.front(), 999);
    EXPECT_TRUE(iptr->base.scalar);
}

TEST(Hdf5Integer, ChunkStream) {
    auto path = "TEST-integer.h5";

    // Simulate multiple chunks so that we test the while{} loop for streaming values.
    const std::size_t nlen = 25000;
    std::vector<std::int32_t> collected(nlen);
    for (std::size_t i = 0; i < nlen; ++i) {
        collected[i] = i;
    }

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "integer");
        write_numbers(vhandle, "data", collected, H5::PredType::NATIVE_INT32, /* chunk_size = */ 82);
    }

    auto parsed = load_hdf5(path, "blub");
    EXPECT_EQ(parsed->type(), uzuki2::INTEGER);
    auto iptr = static_cast<const DefaultIntegerVector*>(parsed.get());
    EXPECT_EQ(iptr->base.values, collected);
}

TEST(Hdf5Integer, ForbiddenType) {
    auto path = "TEST-forbidden.h5";
    std::vector<std::int32_t> data{ 1, 2, 3, 4, 5 };

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "integer");
        write_numbers(vhandle, "data", data, H5::PredType::NATIVE_UINT32);
    }
    expect_hdf5_error(path, "blub", "cannot be represented");

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "integer");
        write_numbers(vhandle, "data", data, H5::PredType::NATIVE_INT64);
    }
    expect_hdf5_error(path, "blub", "cannot be represented by 32-bit");

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto ghandle = vector_opener(handle, "foo", "integer");
        write_numbers(ghandle, "data", data, H5::PredType::NATIVE_DOUBLE);
    }
    expect_hdf5_error(path, "foo", "dataset cannot be represented by 32-bit");

    // Strings are obviously not allowed.
    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "integer");
        write_strings(vhandle, "data", { "foo", "bar" });
    }
    expect_hdf5_error(path, "blub", "cannot be represented by 32-bit");
}

TEST(Hdf5Integer, SmallerType) {
    auto path = "TEST-integer.h5";
    std::vector<std::int32_t> data{ 1, 2, 3, 4, 5 };

    // Smaller types are allowed as long as they fit in the 32-bit signed integer.
    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "integer");
        write_numbers(vhandle, "data", data, H5::PredType::NATIVE_UINT16);
    }

    auto parsed = load_hdf5(path, "blub");
    EXPECT_EQ(parsed->type(), uzuki2::INTEGER);
    auto iptr = static_cast<const DefaultIntegerVector*>(parsed.get());
    EXPECT_EQ(iptr->base.values, data);
}

TEST(Hdf5Integer, LegacyMissing) {
    auto path = "TEST-integer.h5";
    std::vector<std::int32_t> data{ 1, 2, -2147483648, 4, 5 };

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "integer");
        write_numbers(vhandle, "data", data, H5::PredType::NATIVE_INT32);
    }
    {
        auto parsed = load_hdf5(path, "blub");
        EXPECT_EQ(parsed->type(), uzuki2::INTEGER);
        auto iptr = static_cast<const DefaultIntegerVector*>(parsed.get());
        auto modified = data;
        modified[2] = -123456789; // i.e., the test's missing value placeholder.
        EXPECT_EQ(iptr->base.values, modified); 
    }

    // Latest version doesn't automatically use -2**31 to be a placeholder.
    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "integer");
        add_version(vhandle, "1.1");
        write_numbers(vhandle, "data", data, H5::PredType::NATIVE_INT32);
    }
    {
        auto parsed = load_hdf5(path, "blub");
        EXPECT_EQ(parsed->type(), uzuki2::INTEGER);
        auto iptr = static_cast<const DefaultIntegerVector*>(parsed.get());
        EXPECT_EQ(iptr->base.values, data);
    }
}

TEST(Hdf5Integer, MissingPlaceholder) {
    auto path = "TEST-integer.h5";
    std::vector<std::int32_t> data{ 1, -2, 3, -4, 5 };

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "integer");
        add_version(vhandle, "1.1");
        auto dhandle = write_numbers(vhandle, "data", data, H5::PredType::NATIVE_INT32);
        auto ahandle = dhandle.createAttribute("missing-value-placeholder", H5::PredType::NATIVE_INT32, H5S_SCALAR);
        std::int32_t placeholder = 3;
        ahandle.write(H5::PredType::NATIVE_INT32, &placeholder);
    }

    auto parsed = load_hdf5(path, "blub");
    EXPECT_EQ(parsed->type(), uzuki2::INTEGER);
    auto iptr = static_cast<const DefaultIntegerVector*>(parsed.get());
    auto modified = data;
    modified[2] = -123456789; // i.e., the test's missing value placeholder.
    EXPECT_EQ(iptr->base.values, modified);

    // Test coverage of the relevant Dummy class.
    validate_hdf5(path, "blub");
}

TEST(Hdf5Integer, MissingPlaceholderError) {
    auto path = "TEST-integer.h5";
    std::vector<std::int32_t> data{ 1, 2, 3, 4, 5 };

    // Format version < 1.2 only needs the same type class.
    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto ghandle = vector_opener(handle, "foo", "integer");
        add_version(ghandle, "1.1");
        auto dhandle = write_numbers(ghandle, "data", data, H5::PredType::NATIVE_INT32);
        dhandle.createAttribute("missing-value-placeholder", H5::PredType::NATIVE_DOUBLE, H5S_SCALAR);
    }
    expect_hdf5_error(path, "foo", "same type class as");

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto ghandle = vector_opener(handle, "foo", "integer");
        add_version(ghandle, "1.1");
        auto dhandle = write_numbers(ghandle, "data", data, H5::PredType::NATIVE_INT32);
        constexpr hsize_t one = 1;
        dhandle.createAttribute("missing-value-placeholder", H5::PredType::NATIVE_INT32, H5::DataSpace(1, &one));
    }
    expect_hdf5_error(path, "foo", "scalar");

    // Format version >= 1.2 requires the exact same datatype.
    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto ghandle = vector_opener(handle, "foo", "integer");
        add_version(ghandle, "1.2");
        auto dhandle = write_numbers(ghandle, "data", data, H5::PredType::NATIVE_UINT8);
        dhandle.createAttribute("missing-value-placeholder", H5::PredType::NATIVE_INT8, H5S_SCALAR);
    }
    expect_hdf5_error(path, "foo", "same type as");
}
