#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "uzuki2/parse_hdf5.hpp"

#include "utils.h"
#include "../test_subclass.h"

TEST(Hdf5Factor, Simple) {
    auto path = "TEST-factor.h5";
    std::vector<std::int32_t> codes{ 0, 1, 2, 2, 1 };
    std::vector<std::string> levels{ "Albo", "Rudd", "Gillard" };

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "factor");
        write_numbers(vhandle, "data", codes, H5::PredType::NATIVE_INT32);
        write_strings(vhandle, "levels", levels);
    }

    auto parsed = load_hdf5(path, "blub");
    EXPECT_EQ(parsed->type(), uzuki2::FACTOR);
    auto fptr = static_cast<const DefaultFactor*>(parsed.get());
    EXPECT_EQ(fptr->vbase.values, codes);
    EXPECT_EQ(fptr->levels, levels);
    EXPECT_FALSE(fptr->ordered);

    // Test coverage of the relevant Dummy class.
    validate_hdf5(path, "blub");
}

TEST(Hdf5Factor, LegacyOrdered) {
    auto path = "TEST-factor.h5";
    std::vector<std::int32_t> codes{ 1, 2, 0, 0, 2, 1 };
    std::vector<std::string> levels{ "Rudd", "Albo", "Gillard" };

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "ordered");
        write_numbers(vhandle, "data", codes, H5::PredType::NATIVE_INT32);
        write_strings(vhandle, "levels", levels);
    }

    auto parsed = load_hdf5(path, "blub");
    EXPECT_EQ(parsed->type(), uzuki2::FACTOR);
    auto fptr = static_cast<const DefaultFactor*>(parsed.get());
    EXPECT_EQ(fptr->vbase.values, codes);
    EXPECT_EQ(fptr->levels, levels);
    EXPECT_TRUE(fptr->ordered);
}

TEST(Hdf5Factor, Ordered) {
    auto path = "TEST-factor.h5";
    std::vector<std::int32_t> codes{ 1, 2, 0, 0, 2, 1 };
    std::vector<std::string> levels{ "Rudd", "Albo", "Gillard" };

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "factor");
        add_version(vhandle, "1.1");
        write_numbers(vhandle, "data", codes, H5::PredType::NATIVE_INT32);
        write_strings(vhandle, "levels", levels);
        write_number(vhandle, "ordered", 1, H5::PredType::NATIVE_UINT8);
    }
    {
        auto parsed = load_hdf5(path, "blub");
        EXPECT_EQ(parsed->type(), uzuki2::FACTOR);
        auto fptr = static_cast<const DefaultFactor*>(parsed.get());
        EXPECT_EQ(fptr->vbase.values, codes);
        EXPECT_EQ(fptr->levels, levels);
        EXPECT_TRUE(fptr->ordered);
    }

    // Works in the negative case.
    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "factor");
        add_version(vhandle, "1.1");
        write_numbers(vhandle, "data", codes, H5::PredType::NATIVE_INT32);
        write_strings(vhandle, "levels", levels);
        write_number(vhandle, "ordered", 0, H5::PredType::NATIVE_UINT8);
    }
    {
        auto parsed = load_hdf5(path, "blub");
        EXPECT_EQ(parsed->type(), uzuki2::FACTOR);
        auto fptr = static_cast<const DefaultFactor*>(parsed.get());
        EXPECT_EQ(fptr->vbase.values, codes);
        EXPECT_EQ(fptr->levels, levels);
        EXPECT_FALSE(fptr->ordered);
    }
}

TEST(Hdf5Factor, ChunkStream) {
    auto path = "TEST-factor.h5";

    // Simulate multiple chunks so that we test the while{} loop for streaming values.
    // Note that we do so for both the codes and the levels.
    std::vector<std::string> levels{ "ai", "aika", "akari", "akira", "alice", "athena", "alicia" };
    const auto nlevels = levels.size();

    const std::size_t len = 25000;
    std::vector<std::int32_t> collected(len);
    for (std::size_t i = 0; i < len; ++i) {
        collected[i] = i % nlevels;
    }

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto fhandle = vector_opener(handle, "blub", "factor");
        write_numbers(fhandle, "data", collected, H5::PredType::NATIVE_INT32, /* chunk_size = */ 82);
        write_strings(fhandle, "levels", levels, /* variable = */ false, /* chunk_size = */ 3);
    }

    auto parsed = load_hdf5(path, "blub");
    EXPECT_EQ(parsed->type(), uzuki2::FACTOR);
    auto fptr = static_cast<const DefaultFactor*>(parsed.get());
    EXPECT_EQ(fptr->vbase.values, collected);
    EXPECT_EQ(fptr->levels, levels);
}

TEST(Hdf5Factor, ForbiddenType) {
    auto path = "TEST-forbidden.h5";
    std::vector<std::int32_t> data{ 0, 1, 2, 2, 1 };

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "factor");
        write_numbers(vhandle, "data", data, H5::PredType::NATIVE_UINT32);
        write_strings(vhandle, "levels", { "aika", "alice", "athena" });
    }
    expect_hdf5_error(path, "blub", "cannot be represented");

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "factor");
        write_numbers(vhandle, "data", data, H5::PredType::NATIVE_INT64);
        write_strings(vhandle, "levels", { "aika", "alice", "athena" });
    }
    expect_hdf5_error(path, "blub", "cannot be represented by 32-bit");

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "foo", "factor");
        write_numbers(vhandle, "data", data, H5::PredType::NATIVE_DOUBLE);
        write_strings(vhandle, "levels", { "aika", "alice", "athena" });
    }
    expect_hdf5_error(path, "foo", "dataset cannot be represented by 32-bit");

    // Strings are obviously not allowed.
    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "factor");
        write_strings(vhandle, "data", { "foo", "bar" });
        write_strings(vhandle, "levels", { "aika", "alice", "athena" });
    }
    expect_hdf5_error(path, "blub", "cannot be represented by 32-bit");
}

TEST(Hdf5Factor, SmallerType) {
    auto path = "TEST-factor.h5";
    std::vector<std::int32_t> expected_codes { 2, 1, 0, 1, 2 };
    std::vector<std::string> expected_levels { "Albo", "Rudd", "Gillard" };

    // Smaller types are allowed as long as they fit in the 32-bit signed integer.
    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "factor");
        write_numbers(vhandle, "data", expected_codes, H5::PredType::NATIVE_INT8);
        write_strings(vhandle, "levels", expected_levels);
    }

    auto parsed = load_hdf5(path, "blub");
    EXPECT_EQ(parsed->type(), uzuki2::FACTOR);
    auto iptr = static_cast<const DefaultFactor*>(parsed.get());
    EXPECT_EQ(iptr->vbase.values, expected_codes);
    EXPECT_EQ(iptr->levels, expected_levels);
}

TEST(Hdf5Factor, OutOfRange) {
    auto path = "TEST-factor.h5";
    std::vector<std::int32_t> codes { 2, 1, 0, 0, 1, 2 };
    std::vector<std::string> levels { "aika", "alice", "akari" };

    // Invalid code occurs in the first chunk.
    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "foo", "factor");
        auto modified = codes;
        modified[0] = -1;
        write_numbers(vhandle, "data", modified, H5::PredType::NATIVE_INT32, /* chunk_size = */ 2);
        write_strings(vhandle, "levels", levels);
    }
    expect_hdf5_error(path, "foo", "non-negative");

    // Now in the last chunk.
    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "foo", "factor");
        auto modified = codes;
        modified.back() = 3;
        write_numbers(vhandle, "data", modified, H5::PredType::NATIVE_INT32, /* chunk_size = */ 2);
        write_strings(vhandle, "levels", levels);
    }
    expect_hdf5_error(path, "foo", "number of levels");
}

TEST(Hdf5Factor, LevelsError) {
    auto path = "TEST-factor.h5";
    std::vector<std::int32_t> data{ 0, 1, 2, 2, 1 };

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "factor");
        write_numbers(vhandle, "data", data, H5::PredType::NATIVE_INT32);
        write_numbers<std::int32_t>(vhandle, "levels", { 0, 1, 2 }, H5::PredType::NATIVE_UINT8);
    }
    expect_hdf5_error(path, "blub", "UTF-8 string");

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "factor");
        write_numbers(vhandle, "data", data, H5::PredType::NATIVE_INT32);
        write_string(vhandle, "levels", "foobar");
    }
    expect_hdf5_error(path, "blub", "1-dimensional");

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "factor");
        write_numbers(vhandle, "data", data, H5::PredType::NATIVE_INT32);
        write_strings(vhandle, "levels", { "Malcolm", "Malcolm", "John" });
    }
    expect_hdf5_error(path, "blub", "unique");
}

TEST(Hdf5Factor, OrderedError) {
    auto path = "TEST-factor.h5";
    std::vector<std::int32_t> data{ 0, 1, 2, 2, 1 };

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "factor");
        add_version(vhandle, "1.1");

        write_numbers(vhandle, "data", data, H5::PredType::NATIVE_INT32);
        write_strings(vhandle, "levels", { "Malcolm", "Tony", "John" });
        write_number(vhandle, "ordered", 1.2, H5::PredType::NATIVE_DOUBLE);
    }
    expect_hdf5_error(path, "blub", "cannot be represented");

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "factor");
        add_version(vhandle, "1.1");

        write_numbers(vhandle, "data", data, H5::PredType::NATIVE_INT32);
        write_strings(vhandle, "levels", { "Malcolm", "Tony", "John" });
        write_numbers<std::int32_t>(vhandle, "ordered", { 1 }, H5::PredType::NATIVE_INT32);
    }
    expect_hdf5_error(path, "blub", "scalar dataset");
}

TEST(Hdf5Factor, LegacyMissing) {
    auto path = "TEST-factor.h5";
    std::vector<std::int32_t> codes{ 1, 2, -2147483648, 0, -2147483648 };

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "factor");
        write_numbers(vhandle, "data", codes, H5::PredType::NATIVE_INT32);
        write_strings(vhandle, "levels", { "Turnbull", "Morrison", "Abbott" });
    }
    {
        auto parsed = load_hdf5(path, "blub");
        EXPECT_EQ(parsed->type(), uzuki2::FACTOR);
        auto fptr = static_cast<const DefaultFactor*>(parsed.get());
        auto modified = codes;
        modified[2] = -123456789; // i.e., the test's missing value placeholder.
        modified[4] = -123456789;
        EXPECT_EQ(fptr->vbase.values, modified);
    }

    // Legacy placeholder is ignored in later versions.
    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "factor");
        add_version(vhandle, "1.1");
        write_numbers(vhandle, "data", codes, H5::PredType::NATIVE_INT32);
        write_strings(vhandle, "levels", { "Turnbull", "Morrison", "Abbott" });
    }
    expect_hdf5_error(path, "blub", "non-negative");
}

TEST(Hdf5Factor, MissingPlaceholder) {
    const auto path = "TEST-factor.h5";
    std::vector<std::int32_t> codes{ 1, 2, -2147483648, 0, -2147483648 };

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "factor");
        auto dhandle = write_numbers(vhandle, "data", codes, H5::PredType::NATIVE_INT32);
        write_strings(vhandle, "levels", { "Turnbull", "Morrison", "Abbott" });
        auto ahandle = dhandle.createAttribute("missing-value-placeholder", H5::PredType::NATIVE_INT32, H5S_SCALAR);
        std::int32_t placeholder = -2147483648;
        ahandle.write(H5::PredType::NATIVE_INT32, &placeholder);
    }

    auto parsed = load_hdf5(path, "blub");
    EXPECT_EQ(parsed->type(), uzuki2::FACTOR);
    auto fptr = static_cast<const DefaultFactor*>(parsed.get());
    auto modified = codes;
    modified[2] = -123456789; // i.e., the test's missing value placeholder.
    modified[4] = -123456789;
    EXPECT_EQ(fptr->vbase.values, modified);

    // Test coverage of the relevant Dummy class.
    validate_hdf5(path, "blub");
}

TEST(Hdf5Factor, MissingPlaceholderError) {
    auto path = "TEST-factor.h5";
    std::vector<std::int32_t> data{ 1, 0, 0, 1, 1 };

    // Format <1.2 only requires the same datatype class.
    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto ghandle = vector_opener(handle, "foo", "factor");
        add_version(ghandle, "1.1");
        auto dhandle = write_numbers(ghandle, "data", data, H5::PredType::NATIVE_INT32);
        dhandle.createAttribute("missing-value-placeholder", H5::PredType::NATIVE_DOUBLE, H5S_SCALAR);
        write_strings(ghandle, "levels", { "alice", "aika" });
    }
    expect_hdf5_error(path, "foo", "same type class as");

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto ghandle = vector_opener(handle, "foo", "factor");
        add_version(ghandle, "1.1");
        auto dhandle = write_numbers(ghandle, "data", data, H5::PredType::NATIVE_INT32);
        constexpr hsize_t one = 1;
        dhandle.createAttribute("missing-value-placeholder", H5::PredType::NATIVE_INT32, H5::DataSpace(1, &one));
        write_strings(ghandle, "levels", { "alice", "aika" });
    }
    expect_hdf5_error(path, "foo", "scalar");

    // More recent versions require the exact datatype. 
    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto ghandle = vector_opener(handle, "foo", "factor");
        add_version(ghandle, "1.2");
        auto dhandle = write_numbers(ghandle, "data", data, H5::PredType::NATIVE_UINT8);
        dhandle.createAttribute("missing-value-placeholder", H5::PredType::NATIVE_INT8, H5S_SCALAR);
        write_strings(ghandle, "levels", { "alice", "aika" });
    }
    expect_hdf5_error(path, "foo", "same type as");
}
