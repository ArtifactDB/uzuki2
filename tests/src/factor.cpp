#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "uzuki2/parse_hdf5.hpp"

#include "test_subclass.h"
#include "utils.h"

TEST(Hdf5FactorTest, Simple) {
    auto path = "TEST-factor.h5";

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "factor");
        write_numbers<int>(vhandle, "data", { 0, 1, 2, 2, 1 }, H5::PredType::NATIVE_INT32);
        write_strings(vhandle, "levels", { "Albo", "Rudd", "Gillard" });
    }

    auto parsed = load_hdf5(path, "blub");
    EXPECT_EQ(parsed->type(), uzuki2::FACTOR);
    auto fptr = static_cast<const DefaultFactor*>(parsed.get());
    EXPECT_EQ(fptr->size(), 5);
    EXPECT_EQ(fptr->vbase.values.front(), 0);
    EXPECT_EQ(fptr->vbase.values.back(), 1);

    EXPECT_EQ(fptr->levels[0], "Albo");
    EXPECT_EQ(fptr->levels[2], "Gillard");
    EXPECT_FALSE(fptr->ordered);
}

TEST(Hdf5FactorTest, LegacyOrdered) {
    auto path = "TEST-factor.h5";

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "ordered");
        write_numbers<int>(vhandle, "data", { 0, 1, 2, 2, 1 }, H5::PredType::NATIVE_INT32);
        write_strings(vhandle, "levels", { "Albo", "Rudd", "Gillard" });
    }

    auto parsed = load_hdf5(path, "blub");
    EXPECT_EQ(parsed->type(), uzuki2::FACTOR);
    auto fptr = static_cast<const DefaultFactor*>(parsed.get());
    EXPECT_TRUE(fptr->ordered);
}

TEST(Hdf5FactorTest, Ordered) {
    auto path = "TEST-factor.h5";

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "factor");
        add_version(vhandle, "1.1");

        write_numbers<int>(vhandle, "data", { 0, 1, 2, 2, 1 }, H5::PredType::NATIVE_INT32);
        write_strings(vhandle, "levels", { "Albo", "Rudd", "Gillard" });
        write_number(vhandle, "ordered", 1, H5::PredType::NATIVE_UINT8);
    }
    {
        auto parsed = load_hdf5(path, "blub");
        EXPECT_EQ(parsed->type(), uzuki2::FACTOR);
        auto fptr = static_cast<const DefaultFactor*>(parsed.get());
        EXPECT_TRUE(fptr->ordered);
    }

    // Works in the negative case.
    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "factor");
        add_version(vhandle, "1.1");

        write_numbers<int>(vhandle, "data", { 0, 1, 2, 2, 1 }, H5::PredType::NATIVE_INT32);
        write_strings(vhandle, "levels", { "Albo", "Rudd", "Gillard" });
        write_number(vhandle, "ordered", 0, H5::PredType::NATIVE_UINT8);
    }
    {
        auto parsed = load_hdf5(path, "blub");
        EXPECT_EQ(parsed->type(), uzuki2::FACTOR);
        auto fptr = static_cast<const DefaultFactor*>(parsed.get());
        EXPECT_FALSE(fptr->ordered);
    }
}

TEST(Hdf5FactorTest, ChunkStream) {
    auto path = "TEST-factor.h5";

    // Simulate multiple chunks so that we test the while{} loop for streaming values.
    std::vector<std::string> factors{ "ai", "aika", "akari", "akira", "alice", "athena", "alicia" };
    const auto nfactors = factors.size();

    const std::size_t len = 25000;
    std::vector<std::size_t> collected(len);
    for (std::size_t i = 0; i < len; ++i) {
        collected[i] = i % nfactors;
    }

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto fhandle = vector_opener(handle, "blub", "factor");
        write_numbers(fhandle, "data", collected, H5::PredType::NATIVE_INT32, /* chunk_size = */ 82);
        write_strings(fhandle, "levels", factors, /* variable = */ false, /* chunk_size = */ 3);
    }

    auto parsed = load_hdf5(path, "blub");
    EXPECT_EQ(parsed->type(), uzuki2::FACTOR);
    auto fptr = static_cast<const DefaultFactor*>(parsed.get());
    EXPECT_EQ(fptr->vbase.values, collected);
    EXPECT_EQ(fptr->levels, factors);
}

TEST(Hdf5FactorTest, ForbiddenType) {
    auto path = "TEST-forbidden.h5";

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "factor");
        write_numbers<int>(vhandle, "data", { 0, 1, 2, 2, 1 }, H5::PredType::NATIVE_UINT32);
        write_strings(vhandle, "levels", { "aika", "alice", "athena" });
    }
    expect_hdf5_error(path, "blub", "cannot be represented");

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "factor");
        write_numbers<int>(vhandle, "data", { 0, 1, 2, 2, 1 }, H5::PredType::NATIVE_INT64);
        write_strings(vhandle, "levels", { "aika", "alice", "athena" });
    }
    expect_hdf5_error(path, "blub", "cannot be represented by 32-bit");

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "foo", "factor");
        write_numbers<double>(vhandle, "data", { 0, 1, 2, 2, 1 }, H5::PredType::NATIVE_DOUBLE);
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

TEST(Hdf5FactorTest, SmallerType) {
    auto path = "TEST-factor.h5";
    std::vector<std::size_t> expected_codes { 2, 1, 0, 1, 2 };
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

TEST(Hdf5FactorTest, LevelsError) {
    auto path = "TEST-factor.h5";

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "factor");
        write_numbers<int>(vhandle, "data", { 0, 1, 2, 2, 1 }, H5::PredType::NATIVE_INT32);
        write_numbers<int>(vhandle, "levels", { 0, 1, 2 }, H5::PredType::NATIVE_UINT8);
    }
    expect_hdf5_error(path, "blub", "UTF-8 string");

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "factor");
        write_numbers<int>(vhandle, "data", { 0, 1, 2, 2, 1 }, H5::PredType::NATIVE_INT32);
        write_string(vhandle, "levels", "foobar");
    }
    expect_hdf5_error(path, "blub", "1-dimensional");

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "factor");
        write_numbers<int>(vhandle, "data", { 0, 1, 2, 2, 1 }, H5::PredType::NATIVE_INT32);
        write_strings(vhandle, "levels", { "Albo", "Rudd" });
    }
    expect_hdf5_error(path, "blub", "less than the number of levels");

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "factor");
        write_numbers<int>(vhandle, "data", { 0, 1, -1, -1, 1 }, H5::PredType::NATIVE_INT32);
        write_strings(vhandle, "levels", { "Albo", "Rudd" });
    }
    expect_hdf5_error(path, "blub", "non-negative");

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "factor");
        write_numbers<int>(vhandle, "data", { 0, 1, 2, 2, 1 }, H5::PredType::NATIVE_INT32);
        write_strings(vhandle, "levels", { "Malcolm", "Malcolm", "John" });
    }
    expect_hdf5_error(path, "blub", "unique");
}

TEST(Hdf5FactorTest, OrderedError) {
    auto path = "TEST-factor.h5";

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "factor");
        add_version(vhandle, "1.1");

        write_numbers<int>(vhandle, "data", { 0, 1, 2, 2, 1 }, H5::PredType::NATIVE_INT);
        write_strings(vhandle, "levels", { "Malcolm", "Tony", "John" });
        write_number(vhandle, "ordered", 1.2, H5::PredType::NATIVE_DOUBLE);
    }
    expect_hdf5_error(path, "blub", "cannot be represented");

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "factor");
        add_version(vhandle, "1.1");

        write_numbers<int>(vhandle, "data", { 0, 1, 2, 2, 1 }, H5::PredType::NATIVE_INT);
        write_strings(vhandle, "levels", { "Malcolm", "Tony", "John" });
        write_numbers<int>(vhandle, "ordered", { 1 }, H5::PredType::NATIVE_INT);
    }
    expect_hdf5_error(path, "blub", "scalar dataset");
}

TEST(Hdf5FactorTest, LegacyMissing) {
    auto path = "TEST-factor.h5";

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "factor");
        write_numbers<int>(vhandle, "data", { 1, 2, -2147483648, 0, -2147483648 }, H5::PredType::NATIVE_INT32);
        write_strings(vhandle, "levels", { "Turnbull", "Morrison", "Abbott" });
    }

    auto parsed = load_hdf5(path, "blub");
    EXPECT_EQ(parsed->type(), uzuki2::FACTOR);
    auto fptr = static_cast<const DefaultFactor*>(parsed.get());
    EXPECT_EQ(fptr->size(), 5);
    EXPECT_EQ(fptr->vbase.values[2], -1); // i.e., the test's missing value placeholder.
    EXPECT_EQ(fptr->vbase.values[4], -1); 
}

TEST(Hdf5FactorTest, MissingPlaceholder) {
    const auto path = "TEST-factor.h5";

    // Legacy placeholder is ignored in later versions.
    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "factor");
        add_version(vhandle, "1.1");
        write_numbers<int>(vhandle, "data", { 1, 2, -2147483648, 0, -2147483648 }, H5::PredType::NATIVE_INT32);
        write_strings(vhandle, "levels", { "Turnbull", "Morrison", "Abbott" });
    }
    expect_hdf5_error(path, "blub", "non-negative");

    // We can instead set our own placeholder.
    {
        H5::H5File handle(path, H5F_ACC_RDWR);
        auto vhandle = handle.openGroup("blub");
        auto dhandle = vhandle.openDataSet("data");
        auto ahandle = dhandle.createAttribute("missing-value-placeholder", H5::PredType::NATIVE_INT32, H5S_SCALAR);
        int placeholder = -2147483648;
        ahandle.write(H5::PredType::NATIVE_INT32, &placeholder);
    }
    {
        auto parsed = load_hdf5(path, "blub");
        EXPECT_EQ(parsed->type(), uzuki2::FACTOR);
        auto iptr = static_cast<const DefaultFactor*>(parsed.get());
        EXPECT_EQ(iptr->size(), 5);
        EXPECT_EQ(iptr->vbase.values[2], -1); 
        EXPECT_EQ(iptr->vbase.values[4], -1); 
    }
}

TEST(Hdf5FactorTest, MissingPlaceholderError) {
    auto path = "TEST-factor.h5";

    // Format <1.2 only requires the same datatype class.
    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto ghandle = vector_opener(handle, "foo", "factor");
        add_version(ghandle, "1.1");
        auto dhandle = write_numbers<double>(ghandle, "data", { 0, 1, 1, 0, 1 }, H5::PredType::NATIVE_INT32);
        dhandle.createAttribute("missing-value-placeholder", H5::PredType::NATIVE_DOUBLE, H5S_SCALAR);
        write_strings(ghandle, "levels", { "alice", "aika" });
    }
    expect_hdf5_error(path, "foo", "same type class as");

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto ghandle = vector_opener(handle, "foo", "factor");
        add_version(ghandle, "1.1");
        auto dhandle = write_numbers<double>(ghandle, "data", { 1, 0, 0, 1, 1 }, H5::PredType::NATIVE_INT32);
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
        auto dhandle = write_numbers<double>(ghandle, "data", { 1, 1, 0, 0, 0 }, H5::PredType::NATIVE_UINT8);
        dhandle.createAttribute("missing-value-placeholder", H5::PredType::NATIVE_INT8, H5S_SCALAR);
        write_strings(ghandle, "levels", { "alice", "aika" });
    }
    expect_hdf5_error(path, "foo", "same type as");
}

TEST(JsonFactorTest, SimpleLoading) {
    {
        auto parsed = load_json("{ \"type\": \"factor\", \"values\": [ 0, 1, 1, 0, 2 ], \"levels\": [ \"akari\", \"alice\", \"aika\" ] }");
        EXPECT_EQ(parsed->type(), uzuki2::FACTOR);

        auto fptr = static_cast<const DefaultFactor*>(parsed.get());
        EXPECT_EQ(fptr->size(), 5);
        EXPECT_EQ(fptr->vbase.values.front(), 0);
        EXPECT_EQ(fptr->vbase.values.back(), 2);

        EXPECT_EQ(fptr->levels[0], "akari");
        EXPECT_EQ(fptr->levels[2], "aika");
    }

    // Works in later versions.
    {
        auto parsed = load_json("{ \"type\": \"factor\", \"values\": [ 2, 1, 0 ], \"levels\": [ \"athena\", \"akira\", \"alicia\" ], \"version\": \"1.1\" }");
        EXPECT_EQ(parsed->type(), uzuki2::FACTOR);
        auto fptr = static_cast<const DefaultFactor*>(parsed.get());
        EXPECT_FALSE(fptr->ordered);
    }


    /********************************************
     *** See integer.cpp for tests for names. ***
     ********************************************/
}

TEST(JsonFactorTest, OrderedLoading) {
    {
        auto parsed = load_json("{ \"type\": \"ordered\", \"values\": [ 2, 1, 0 ], \"levels\": [ \"athena\", \"akira\", \"alicia\" ] }");
        EXPECT_EQ(parsed->type(), uzuki2::FACTOR);
        auto fptr = static_cast<const DefaultFactor*>(parsed.get());
        EXPECT_TRUE(fptr->ordered);
    }

    // Works in later versions.
    {
        auto parsed = load_json("{ \"type\": \"factor\", \"values\": [ 2, 1, 0 ], \"levels\": [ \"athena\", \"akira\", \"alicia\" ], \"ordered\": true, \"version\": \"1.1\" }");
        EXPECT_EQ(parsed->type(), uzuki2::FACTOR);
        auto fptr = static_cast<const DefaultFactor*>(parsed.get());
        EXPECT_TRUE(fptr->ordered);
    }

    // Responds to the negative case.
    {
        auto parsed = load_json("{ \"type\": \"factor\", \"values\": [ 2, 1, 0 ], \"levels\": [ \"athena\", \"akira\", \"alicia\" ], \"ordered\": false, \"version\": \"1.1\" }");
        EXPECT_EQ(parsed->type(), uzuki2::FACTOR);
        auto fptr = static_cast<const DefaultFactor*>(parsed.get());
        EXPECT_FALSE(fptr->ordered);
    }
}

TEST(JsonFactorTest, MissingValues) {
    {
        auto parsed = load_json("{ \"type\": \"ordered\", \"values\": [ 2, 1, -2147483648, 0, null ], \"levels\": [ \"athena\", \"akira\", \"alicia\" ] }");
        EXPECT_EQ(parsed->type(), uzuki2::FACTOR);
        auto fptr = static_cast<const DefaultFactor*>(parsed.get());
        EXPECT_EQ(fptr->vbase.values[2], -1); // i.e., the test's missing value placeholder.
        EXPECT_EQ(fptr->vbase.values[4], -1); 
    }

    // Special value doesn't work in the latest version.
   expect_json_error("{ \"type\": \"factor\", \"values\": [ 2, 1, -2147483648, 0, null ], \"levels\": [ \"athena\", \"akira\", \"alicia\" ], \"version\":\"1.1\" }", "out of range");

    {
        auto parsed = load_json("{ \"type\": \"factor\", \"values\": [ 2, 1, null, 0, null ], \"levels\": [ \"athena\", \"akira\", \"alicia\" ], \"version\": \"1.1\" }");
        EXPECT_EQ(parsed->type(), uzuki2::FACTOR);
        auto fptr = static_cast<const DefaultFactor*>(parsed.get());
        EXPECT_EQ(fptr->vbase.values[2], -1); // i.e., the test's missing value placeholder.
        EXPECT_EQ(fptr->vbase.values[4], -1); 
    }
}

TEST(JsonFactorTest, CheckError) {
    expect_json_error("{ \"type\": \"ordered\", \"values\": [ true, 0 ], \"levels\": [ \"athena\", \"akira\", \"alicia\" ] }", "expected a number");
    expect_json_error("{ \"type\": \"ordered\", \"values\": [ 1.2, 0 ], \"levels\": [ \"athena\", \"akira\", \"alicia\" ] }", "expected an integer");

    expect_json_error("{ \"type\": \"ordered\", \"values\": [ 2, 1, 3, 0 ], \"levels\": [ \"athena\", \"akira\", \"alicia\" ] }", "out of range");
    expect_json_error("{ \"type\": \"ordered\", \"values\": [ 2, 1, -1, 0 ], \"levels\": [ \"athena\", \"akira\", \"alicia\" ] }", "out of range");
    expect_json_error("{ \"type\": \"ordered\", \"values\": [ 2, 1, 0 ], \"levels\": [ \"aria\", \"aria\", \"aria\" ] }", "duplicate string");

    expect_json_error("{ \"type\": \"factor\", \"values\": [ 1, 0 ], \"levels\": [ \"athena\", \"akira\", \"alicia\" ], \"ordered\": 1, \"version\": \"1.1\" }", "expected a boolean");
    expect_json_error("{ \"type\": \"ordered\", \"values\": [ 1, 0 ], \"levels\": [ \"athena\", \"akira\", \"alicia\" ], \"version\": \"1.1\" }", "unknown object type 'ordered'");

    /***********************************************
     *** See integer.cpp for vector error tests. ***
     ***********************************************/
}
