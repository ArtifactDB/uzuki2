#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "uzuki2/parse_hdf5.hpp"

#include "utils.h"
#include "../test_subclass.h"

TEST(Hdf5String, FixedVector) {
    auto path = "TEST-string.h5";
    std::vector<std::string> data{ "foo", "whee", "stuff" };

    // Simple stuff works correctly.
    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "string");
        write_strings(vhandle, "data", data);
    }
    {
        auto parsed = load_hdf5(path, "blub");
        EXPECT_EQ(parsed->type(), uzuki2::STRING);
        auto sptr = static_cast<const DefaultStringVector*>(parsed.get());
        EXPECT_EQ(sptr->base.values, data);
        EXPECT_EQ(sptr->format, uzuki2::StringVector::NONE);
    }

    // Also testing some more recent versions to get some coverage of the no-'format' case.
    {
        H5::H5File handle(path, H5F_ACC_RDWR);
        auto vhandle = handle.openGroup("blub");
        add_version(vhandle, "1.1");
    }
    {
        auto parsed = load_hdf5(path, "blub");
        EXPECT_EQ(parsed->type(), uzuki2::STRING);
        auto sptr = static_cast<const DefaultStringVector*>(parsed.get());
        EXPECT_EQ(sptr->base.values, data);
        EXPECT_EQ(sptr->format, uzuki2::StringVector::NONE);
    }
}

TEST(Hdf5String, VariableVector) {
    auto path = "TEST-string.h5";
    std::vector<std::string> data{ "foo-qwerty", "whee", "stuff-asdasd" };

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "string");
        write_strings(vhandle, "data", data, /* variable = */ true, /* chunk_size = */ 0);
    }

    auto parsed = load_hdf5(path, "blub");
    EXPECT_EQ(parsed->type(), uzuki2::STRING);
    auto sptr = static_cast<const DefaultStringVector*>(parsed.get());
    EXPECT_EQ(sptr->base.values, data);
    EXPECT_EQ(sptr->format, uzuki2::StringVector::NONE);
}

TEST(Hdf5String, FixedScalar) {
    auto path = "TEST-string.h5";

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "string");
        write_string(vhandle, "data", "antony");
    }

    auto parsed = load_hdf5(path, "blub");
    EXPECT_EQ(parsed->type(), uzuki2::STRING);
    auto sptr = static_cast<const DefaultStringVector*>(parsed.get());
    EXPECT_EQ(sptr->size(), 1);
    EXPECT_EQ(sptr->base.values.front(), "antony");
    EXPECT_TRUE(sptr->base.scalar);
    EXPECT_EQ(sptr->format, uzuki2::StringVector::NONE);
}

TEST(Hdf5String, VariableScalar) {
    auto path = "TEST-string.h5";

    // Scalars work correctly.
    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "string");
        write_string(vhandle, "data", "cleopatra", /* variable = */ true);
    }

    auto parsed = load_hdf5(path, "blub");
    EXPECT_EQ(parsed->type(), uzuki2::STRING);
    auto sptr = static_cast<const DefaultStringVector*>(parsed.get());
    EXPECT_EQ(sptr->size(), 1);
    EXPECT_EQ(sptr->base.values.front(), "cleopatra");
    EXPECT_TRUE(sptr->base.scalar);
    EXPECT_EQ(sptr->format, uzuki2::StringVector::NONE);
}

TEST(Hdf5String, BlockLoading) {
    auto path = "TEST-string.h5";

    // Simulate multiple chunks so that we test the while{} loop for streaming values.
    const std::size_t len = 25000;
    std::vector<std::string> collected(len);
    for (std::size_t i = 0; i < len; ++i) {
        collected[i] = std::to_string(i);
    }

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "string");
        write_strings(vhandle, "data", collected, /* variable = */ false, /* chunk_size = */ 999);
    }

    auto parsed = load_hdf5(path, "blub");
    EXPECT_EQ(parsed->type(), uzuki2::STRING);
    auto sptr = static_cast<const DefaultStringVector*>(parsed.get());
    EXPECT_EQ(sptr->base.values, collected);
}

TEST(Hdf5String, ForbiddenType) {
    auto path = "TEST-string.h5";

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto ghandle = vector_opener(handle, "foo", "string");
        write_numbers<std::int32_t>(ghandle, "data", { 1, 2, 3, 4, 5 }, H5::PredType::NATIVE_INT32);
    }
    expect_hdf5_error(path, "foo", "can be represented by a UTF-8 string");
}

TEST(Hdf5String, MissingPlaceholder) {
    auto path = "TEST-string.h5";
    std::vector<std::string> data{ "michael", "gabriel", "raphael", "lucifer" };

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "string");
        auto dhandle = write_strings(vhandle, "data", data);

        H5::StrType stype(0, H5T_VARIABLE);
        auto ahandle = dhandle.createAttribute("missing-value-placeholder", stype, H5S_SCALAR);
        std::string target = "lucifer";
        ahandle.write(stype, target);
    }

    auto parsed = load_hdf5(path, "blub");
    EXPECT_EQ(parsed->type(), uzuki2::STRING);
    auto sptr = static_cast<const DefaultStringVector*>(parsed.get());
    auto modified = data;
    modified[3] = "ich bin missing"; // i.e., the test's missing value placeholder.
    EXPECT_EQ(sptr->base.values, modified);
}

TEST(Hdf5String, MissingPlaceholderError) {
    auto path = "TEST-string.h5";

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto ghandle = vector_opener(handle, "foo", "string");
        auto dhandle = write_strings(ghandle, "data", { "michael", "gabriel", "raphael", "lucifer" });
        dhandle.createAttribute("missing-value-placeholder", H5::PredType::NATIVE_DOUBLE, H5S_SCALAR);
    }
    expect_hdf5_error(path, "foo", "attribute to be a UTF-8 string");

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto ghandle = vector_opener(handle, "foo", "string");
        auto dhandle = write_strings(ghandle, "data", { "michael", "gabriel", "raphael", "lucifer" });
        constexpr hsize_t one = 1;
        H5::DataSpace aspace(1, &one);
        dhandle.createAttribute("missing-value-placeholder", H5::StrType(0, H5T_VARIABLE), aspace);
    }
    expect_hdf5_error(path, "foo", "scalar");
}
