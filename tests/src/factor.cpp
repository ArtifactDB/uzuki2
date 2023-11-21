#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "uzuki2/parse_hdf5.hpp"

#include "test_subclass.h"
#include "utils.h"

TEST(Hdf5FactorTest, SimpleLoading) {
    auto path = "TEST-factor.h5";

    // Simple stuff works correctly.
    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "factor");
        create_dataset<int>(vhandle, "data", { 0, 1, 2, 2, 1 }, H5::PredType::NATIVE_INT);
        create_dataset(vhandle, "levels", { "Albo", "Rudd", "Gillard" });
    }
    {
        auto parsed = load_hdf5(path, "blub");
        EXPECT_EQ(parsed->type(), uzuki2::FACTOR);
        auto fptr = static_cast<const DefaultFactor*>(parsed.get());
        EXPECT_EQ(fptr->size(), 5);
        EXPECT_EQ(fptr->vbase.values.front(), 0);
        EXPECT_EQ(fptr->vbase.values.back(), 1);

        EXPECT_EQ(fptr->levels[0], "Albo");
        EXPECT_EQ(fptr->levels[2], "Gillard");
    }

    // Works in later verisons.
    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "factor");
        add_version(vhandle, "1.1");

        create_dataset<int>(vhandle, "data", { 0, 1, 2, 2, 1 }, H5::PredType::NATIVE_INT);
        create_dataset(vhandle, "levels", { "Albo", "Rudd", "Gillard" });
    }
    {
        auto parsed = load_hdf5(path, "blub");
        EXPECT_EQ(parsed->type(), uzuki2::FACTOR);
        auto fptr = static_cast<const DefaultFactor*>(parsed.get());
        EXPECT_FALSE(fptr->ordered);
    }

    /********************************************
     *** See integer.cpp for tests for names. ***
     ********************************************/
}

TEST(Hdf5FactorTest, OrderedLoading) {
    auto path = "TEST-factor.h5";

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "ordered");
        create_dataset<int>(vhandle, "data", { 0, 1, 2, 2, 1 }, H5::PredType::NATIVE_INT);
        create_dataset(vhandle, "levels", { "Albo", "Rudd", "Gillard" });
    }
    {
        auto parsed = load_hdf5(path, "blub");
        EXPECT_EQ(parsed->type(), uzuki2::FACTOR);
        auto fptr = static_cast<const DefaultFactor*>(parsed.get());
        EXPECT_TRUE(fptr->ordered);
    }

    // Works with later versions.
    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "factor");
        add_version(vhandle, "1.1");

        create_dataset<int>(vhandle, "data", { 0, 1, 2, 2, 1 }, H5::PredType::NATIVE_INT);
        create_dataset(vhandle, "levels", { "Albo", "Rudd", "Gillard" });
        write_scalar(vhandle, "ordered", 1, H5::PredType::NATIVE_UINT8);
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

        create_dataset<int>(vhandle, "data", { 0, 1, 2, 2, 1 }, H5::PredType::NATIVE_INT);
        create_dataset(vhandle, "levels", { "Albo", "Rudd", "Gillard" });
        write_scalar(vhandle, "ordered", 0, H5::PredType::NATIVE_UINT8);
    }
    {
        auto parsed = load_hdf5(path, "blub");
        EXPECT_EQ(parsed->type(), uzuki2::FACTOR);
        auto fptr = static_cast<const DefaultFactor*>(parsed.get());
        EXPECT_FALSE(fptr->ordered);
    }

}

TEST(Hdf5FactorTest, MissingValues) {
    auto path = "TEST-factor.h5";

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "factor");
        create_dataset<int>(vhandle, "data", { 1, 2, -2147483648, 0, -2147483648 }, H5::PredType::NATIVE_INT);
        create_dataset(vhandle, "levels", { "Turnbull", "Morrison", "Abbott" });
    }
    {
        auto parsed = load_hdf5(path, "blub");
        EXPECT_EQ(parsed->type(), uzuki2::FACTOR);
        auto fptr = static_cast<const DefaultFactor*>(parsed.get());
        EXPECT_EQ(fptr->size(), 5);
        EXPECT_EQ(fptr->vbase.values[2], -1); // i.e., the test's missing value placeholder.
        EXPECT_EQ(fptr->vbase.values[4], -1); 
    }

    // Placeholder is ignored in later versions.
    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "factor");
        add_version(vhandle, "1.1");
        create_dataset<int>(vhandle, "data", { 1, 2, -2147483648, 0, -2147483648 }, H5::PredType::NATIVE_INT);
        create_dataset(vhandle, "levels", { "Turnbull", "Morrison", "Abbott" });
    }
    expect_hdf5_error(path, "blub", "non-negative");

    // We can instead set our own placeholder.
    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "factor");
        add_version(vhandle, "1.1");

        auto dhandle = create_dataset<int>(vhandle, "data", { 1, 2, -2147483648, 0, -2147483648 }, H5::PredType::NATIVE_INT);
        auto ahandle = dhandle.createAttribute("missing-value-placeholder", H5::PredType::NATIVE_INT, H5S_SCALAR);
        int placeholder = -2147483648;
        ahandle.write(H5::PredType::NATIVE_INT, &placeholder);

        create_dataset(vhandle, "levels", { "Turnbull", "Morrison", "Abbott" });
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

TEST(Hdf5FactorTest, CheckError) {
    auto path = "TEST-factor.h5";

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "factor");
        create_dataset<int>(vhandle, "data", { 0, 1, 2, 2, 1 }, H5::PredType::NATIVE_INT);
        create_dataset(vhandle, "levels", { "Albo", "Rudd" });
    }
    expect_hdf5_error(path, "blub", "less than the number of levels");

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "factor");
        create_dataset<int>(vhandle, "data", { 0, 1, -1, -1, 1 }, H5::PredType::NATIVE_INT);
        create_dataset(vhandle, "levels", { "Albo", "Rudd" });
    }
    expect_hdf5_error(path, "blub", "non-negative");

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "factor");
        create_dataset<int>(vhandle, "data", { 0, 1, 2, 2, 1 }, H5::PredType::NATIVE_INT);
        create_dataset(vhandle, "levels", { "Malcolm", "Malcolm", "John" });
    }
    expect_hdf5_error(path, "blub", "unique");

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "factor");
        add_version(vhandle, "1.1");

        create_dataset<int>(vhandle, "data", { 0, 1, 2, 2, 1 }, H5::PredType::NATIVE_INT);
        create_dataset(vhandle, "levels", { "Malcolm", "Tony", "John" });
        vhandle.createGroup("ordered");
    }
    expect_hdf5_error(path, "blub", "dataset");

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "factor");
        add_version(vhandle, "1.1");

        create_dataset<int>(vhandle, "data", { 0, 1, 2, 2, 1 }, H5::PredType::NATIVE_INT);
        create_dataset(vhandle, "levels", { "Malcolm", "Tony", "John" });
        write_scalar(vhandle, "ordered", 1.2, H5::PredType::NATIVE_DOUBLE);
    }
    expect_hdf5_error(path, "blub", "cannot be represented");

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "factor");
        add_version(vhandle, "1.1");

        create_dataset<int>(vhandle, "data", { 0, 1, 2, 2, 1 }, H5::PredType::NATIVE_INT);
        create_dataset(vhandle, "levels", { "Malcolm", "Tony", "John" });
        create_dataset<int>(vhandle, "ordered", { 1 }, H5::PredType::NATIVE_INT);
    }
    expect_hdf5_error(path, "blub", "scalar dataset");

    /***********************************************
     *** See integer.cpp for vector error tests. ***
     ***********************************************/
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
