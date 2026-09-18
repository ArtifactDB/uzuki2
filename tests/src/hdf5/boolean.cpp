#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <cstddef>
#include <vector>

#include "uzuki2/parse_hdf5.hpp"

#include "utils.h"
#include "../test_subclass.h"

TEST(Hdf5Boolean, Vector) {
    auto path = "TEST-boolean.h5";

    std::vector<std::uint8_t> data{ 1, 0, 1, 0, 0 };
    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "boolean");
        write_numbers(vhandle, "data", data, H5::PredType::NATIVE_INT32);
    }

    auto parsed = load_hdf5(path, "blub");
    EXPECT_EQ(parsed->type(), uzuki2::BOOLEAN);
    auto bptr = static_cast<const DefaultBooleanVector*>(parsed.get());
    EXPECT_EQ(bptr->size(), 5);
    EXPECT_EQ(bptr->base.values, data);
    EXPECT_FALSE(bptr->base.scalar);

    // Test coverage of the relevant Dummy class.
    validate_hdf5(path, "blub");
}

TEST(Hdf5Boolean, Scalar) {
    auto path = "TEST-boolean.h5";

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "boolean");
        write_number(vhandle, "data", 1, H5::PredType::NATIVE_INT32);
    }

    auto parsed = load_hdf5(path, "blub");
    EXPECT_EQ(parsed->type(), uzuki2::BOOLEAN);
    auto bptr = static_cast<const DefaultBooleanVector*>(parsed.get());
    EXPECT_EQ(bptr->size(), 1);
    EXPECT_EQ(bptr->base.values.front(), 1);
    EXPECT_TRUE(bptr->base.scalar);
}

TEST(Hdf5Boolean, ChunkStream) {
    auto path = "TEST-boolean.h5";

    // Simulate multiple chunks so that we test the while{} loop for streaming values.
    const std::size_t len = 25000;
    std::vector<std::uint8_t> collected(len);
    for (std::size_t i = 0; i < len; ++i) {
        collected[i] = i % 2;
    }

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "boolean");
        write_numbers(vhandle, "data", collected, H5::PredType::NATIVE_INT32, /* chunk_size = */ 379);
    }

    auto parsed = load_hdf5(path, "blub");
    EXPECT_EQ(parsed->type(), uzuki2::BOOLEAN);
    auto iptr = static_cast<const DefaultBooleanVector*>(parsed.get());
    EXPECT_EQ(iptr->base.values, collected);
}

TEST(Hdf5Boolean, ForbiddenType) {
    auto path = "TEST-forbidden.h5";
    std::vector<std::int32_t> data{ 0, 1, 0, 0, 1 };

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "boolean");
        write_numbers(vhandle, "data", data, H5::PredType::NATIVE_UINT32);
    }
    expect_hdf5_error(path, "blub", "cannot be represented");

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "boolean");
        write_numbers(vhandle, "data", data, H5::PredType::NATIVE_INT64);
    }
    expect_hdf5_error(path, "blub", "cannot be represented by 32-bit");

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto ghandle = vector_opener(handle, "foo", "boolean");
        write_numbers(ghandle, "data", data, H5::PredType::NATIVE_DOUBLE);
    }
    expect_hdf5_error(path, "foo", "dataset cannot be represented by 32-bit");

    // Strings are obviously not allowed.
    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "boolean");
        write_strings(vhandle, "data", { "foo", "bar" });
        add_version(handle.openGroup("blub"), "1.3");
    }
    expect_hdf5_error(path, "blub", "cannot be represented by 32-bit");
}

TEST(Hdf5Boolean, SmallerType) {
    auto path = "TEST-boolean.h5";
    std::vector<std::uint8_t> data{ 1, 0, 1, 0 };

    // Smaller types are allowed as long as they fit in the 32-bit signed integer.
    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "boolean");
        write_numbers(vhandle, "data", data, H5::PredType::NATIVE_UINT8);
    }

    auto parsed = load_hdf5(path, "blub");
    EXPECT_EQ(parsed->type(), uzuki2::BOOLEAN);
    auto iptr = static_cast<const DefaultBooleanVector*>(parsed.get());
    EXPECT_EQ(iptr->base.values, data);
}

TEST(Hdf5Boolean, LegacyMissing) {
    auto path = "TEST-boolean.h5";
    std::vector<std::int32_t> data{ 1, 0, -2147483648, 0, 1 };

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "boolean");
        write_numbers(vhandle, "data", data, H5::PredType::NATIVE_INT32);
    }
    {
        auto parsed = load_hdf5(path, "blub");
        EXPECT_EQ(parsed->type(), uzuki2::BOOLEAN);
        auto bptr = static_cast<const DefaultBooleanVector*>(parsed.get());
        auto expected = data;
        expected[2] = 255; // i.e., the test's missing value placeholder.
        EXPECT_EQ(bptr->base.values, std::vector<std::uint8_t>(expected.begin(), expected.end()));
    }

    // Later versions do not recognize -2^31 as a special value.
    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "boolean");
        add_version(vhandle, "1.1");
        write_numbers<std::int32_t>(vhandle, "data", { 1, 0, -2147483648, 0, 1 }, H5::PredType::NATIVE_INT32);
    }
    expect_hdf5_error(path, "blub", "boolean values should be");
}

TEST(Hdf5Boolean, MissingPlaceholder) {
    auto path = "TEST-boolean.h5";
    std::vector<std::int32_t> data{ 1, 0, 2, 0, 1 };

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "boolean");
        add_version(vhandle, "1.1");
        auto dhandle = write_numbers(vhandle, "data", data, H5::PredType::NATIVE_UINT8);
        auto ahandle = dhandle.createAttribute("missing-value-placeholder", H5::PredType::NATIVE_INT32, H5S_SCALAR);
        std::int32_t placeholder = 2;
        ahandle.write(H5::PredType::NATIVE_INT32, &placeholder);
    }

    auto parsed = load_hdf5(path, "blub");
    EXPECT_EQ(parsed->type(), uzuki2::BOOLEAN);
    auto bptr = static_cast<const DefaultBooleanVector*>(parsed.get());
    auto expected = data;
    expected[2] = 255; // i.e., the test's missing value placeholder.
    EXPECT_EQ(bptr->base.values, std::vector<std::uint8_t>(expected.begin(), expected.end()));

    // Test coverage of the relevant Dummy class.
    validate_hdf5(path, "blub");
}

TEST(Hdf5Boolean, MissingPlaceholderError) {
    auto path = "TEST-integer.h5";
    std::vector<std::int32_t> data{ 0, 1, 0, 0, 1 };

    // Format version < 1.2 only needs the same type class.
    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto ghandle = vector_opener(handle, "foo", "boolean");
        add_version(ghandle, "1.1");
        auto dhandle = write_numbers(ghandle, "data", data, H5::PredType::NATIVE_INT32);
        dhandle.createAttribute("missing-value-placeholder", H5::PredType::NATIVE_DOUBLE, H5S_SCALAR);
    }
    expect_hdf5_error(path, "foo", "same type class as");

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto ghandle = vector_opener(handle, "foo", "boolean");
        add_version(ghandle, "1.1");
        auto dhandle = write_numbers(ghandle, "data", data, H5::PredType::NATIVE_INT32);
        constexpr hsize_t one = 1;
        dhandle.createAttribute("missing-value-placeholder", H5::PredType::NATIVE_INT32, H5::DataSpace(1, &one));
    }
    expect_hdf5_error(path, "foo", "scalar");

    // Format version >= 1.2 requires the exact same datatype.
    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto ghandle = vector_opener(handle, "foo", "boolean");
        add_version(ghandle, "1.2");
        auto dhandle = write_numbers(ghandle, "data", data, H5::PredType::NATIVE_UINT8);
        dhandle.createAttribute("missing-value-placeholder", H5::PredType::NATIVE_INT8, H5S_SCALAR);
    }
    expect_hdf5_error(path, "foo", "same type as");
}

TEST(Hdf5Boolean, OutOfRangeVector) {
    auto path = "TEST-boolean.h5";

    std::vector<std::int32_t> collected(25000);
    for (size_t i = 0; i < collected.size(); ++i) {
        collected[i] = i % 2;
    }

    // Injecting invalid values at the front and the back, to confirm that we test all entries for validity.
    {
        collected.front() = 100;
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto ghandle = vector_opener(handle, "foo", "boolean");
        write_numbers(ghandle, "data", collected, H5::PredType::NATIVE_INT32, /* chunk_size = */ 1239);
        collected.front() = 0;
    }
    expect_hdf5_error(path, "foo", "boolean values should be");

    {
        collected.back() = 100;
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto ghandle = vector_opener(handle, "foo", "boolean");
        write_numbers(ghandle, "data", collected, H5::PredType::NATIVE_INT32, /* chunk_size = */ 1239);
        collected.back() = 0;
    }
    expect_hdf5_error(path, "foo", "boolean values should be");
}

TEST(Hdf5Boolean, OutOfRangeScalar) {
    auto path = "TEST-boolean.h5";

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "boolean");
        write_number(vhandle, "data", 100, H5::PredType::NATIVE_INT8);
    }
    expect_hdf5_error(path, "blub", "boolean values should be");
}
