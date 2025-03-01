#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "uzuki2/parse_hdf5.hpp"

#include "utils.h"

TEST(Hdf5VlsTest, Basic) {
    auto path = "TEST-vls.h5";
    std::string heap = "abcdefghijklmno";
    size_t nlen = 10;

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "vls");
        add_version(vhandle, "1.4");

        auto hhandle = create_dataset(vhandle, "heap", heap.size(), H5::PredType::NATIVE_UINT8);
        const unsigned char* hptr = reinterpret_cast<const unsigned char*>(heap.c_str());
        hhandle.write(hptr, H5::PredType::NATIVE_UCHAR);

        std::vector<ritsuko::hdf5::vls::Pointer<uint64_t, uint64_t> > pointers(nlen);
        size_t n = 0;
        for (size_t i = 0; i < nlen; ++i) {
            pointers[i].offset = n;
            size_t count = (i % 2) + 1; // for some interesting differences.
            pointers[i].length = count;
            n += count;
        }
        auto ptype = ritsuko::hdf5::vls::define_pointer_datatype<uint64_t, uint64_t>();
        auto phandle = create_dataset(vhandle, "data", pointers.size(), ptype);
        phandle.write(pointers.data(), ptype);
    }

    // Check that it works correctly.
    {
        auto parsed = load_hdf5(path, "blub");
        EXPECT_EQ(parsed->type(), uzuki2::STRING);
        auto sptr = static_cast<const DefaultStringVector*>(parsed.get());
        EXPECT_EQ(sptr->size(), nlen);
        std::vector<std::string> expected { "a", "bc", "d", "ef", "g", "hi", "j", "kl", "m", "no" };
        EXPECT_EQ(sptr->base.values, expected);
    }

    // Adding a missing value placeholder.
    {
        {
            H5::H5File handle(path, H5F_ACC_RDWR);
            auto vhandle = handle.openDataSet("blub/data");
            H5::StrType stype(0, H5T_VARIABLE);
            auto ahandle = vhandle.createAttribute("missing-value-placeholder", stype, H5S_SCALAR);
            ahandle.write(stype, std::string("hi"));
        }

        auto parsed = load_hdf5(path, "blub");
        EXPECT_EQ(parsed->type(), uzuki2::STRING);
        auto sptr = static_cast<const DefaultStringVector*>(parsed.get());
        EXPECT_EQ(sptr->base.values[5], "ich bin missing"); // the test's missing placeholder.

        // Adding the wrong missing value placeholder.
        {
            H5::H5File handle(path, H5F_ACC_RDWR);
            auto vhandle = handle.openDataSet("blub/data");
            vhandle.removeAttr("missing-value-placeholder");
            vhandle.createAttribute("missing-value-placeholder", H5::PredType::NATIVE_INT, H5S_SCALAR);
        }
        expect_hdf5_error(path, "blub", "string datatype");

        // Removing for the next checks.
        {
            H5::H5File handle(path, H5F_ACC_RDWR);
            auto vhandle = handle.openDataSet("blub/data");
            vhandle.removeAttr("missing-value-placeholder");
        }
    }
}

TEST(Hdf5VlsTest, Failures) {
    auto path = "TEST-vls.h5";
    std::string heap = "abcdefghijklmno";
    size_t nlen = 10;

    // Shortening the heap to check that we perform bounds checks on the pointers.
    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto ghandle = vector_opener(handle, "blub", "vls");
        add_version(ghandle, "1.4");

        hsize_t zero = 0;
        H5::DataSpace hspace(1, &zero);
        ghandle.createDataSet("heap", H5::PredType::NATIVE_UINT8, hspace);

        std::vector<ritsuko::hdf5::vls::Pointer<uint64_t, uint64_t> > pointers(nlen);
        for (size_t i = 0; i < nlen; ++i) {
            pointers[i].offset = i;
            pointers[i].length = 1;
        }
        auto ptype = ritsuko::hdf5::vls::define_pointer_datatype<uint64_t, uint64_t>();
        auto phandle = create_dataset(ghandle, "data", pointers.size(), ptype);
        phandle.write(pointers.data(), ptype);
    }
    expect_hdf5_error(path, "blub", "out of range");

    // Checking that we check for 64-bit unsigned integer types. 
    {
        H5::H5File handle(path, H5F_ACC_RDWR);
        auto ghandle = handle.openGroup("blub");
        ghandle.unlink("data");

        std::vector<ritsuko::hdf5::vls::Pointer<int, int> > pointers(3);
        for (auto& p : pointers) {
            p.offset = 0;
            p.length = 0;
        }
        hsize_t plen = pointers.size();
        H5::DataSpace pspace(1, &plen);
        auto ptype = ritsuko::hdf5::vls::define_pointer_datatype<int, int>();
        auto phandle = ghandle.createDataSet("data", ptype, pspace);
        phandle.write(pointers.data(), ptype);
    }
    expect_hdf5_error(path, "blub", "64-bit unsigned integer");

    // Checking that this only works in the latest version.
    {
        H5::H5File handle(path, H5F_ACC_RDWR);
        auto vhandle = handle.openGroup("blub");
        vhandle.removeAttr("uzuki_version");
    }
    expect_hdf5_error(path, "blub", "unknown vector type");
}

TEST(Hdf5VlsTest, Scalar) {
    auto path = "TEST-vls.h5";
    std::string heap = "abcdefghijklmno";

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto ghandle = vector_opener(handle, "blub", "vls");
        add_version(ghandle, "1.4");

        auto hhandle = create_dataset(ghandle, "heap", heap.size(), H5::PredType::NATIVE_UINT8);
        const unsigned char* hptr = reinterpret_cast<const unsigned char*>(heap.c_str());
        hhandle.write(hptr, H5::PredType::NATIVE_UCHAR);

        ritsuko::hdf5::vls::Pointer<uint8_t, uint8_t> ptr;
        ptr.offset = 0; ptr.length = 10;
        auto ptype = ritsuko::hdf5::vls::define_pointer_datatype<uint8_t, uint8_t>();
        auto phandle = ghandle.createDataSet("data", ptype, H5S_SCALAR);
        phandle.write(&ptr, ptype);
    }
    {
        auto parsed = load_hdf5(path, "blub");
        EXPECT_EQ(parsed->type(), uzuki2::STRING);
        auto sptr = static_cast<const DefaultStringVector*>(parsed.get());
        EXPECT_EQ(sptr->size(), 1);
        EXPECT_EQ(sptr->base.values.front(), "abcdefghij");
    }

    // Checking that it works correctly with early termination.
    {
        H5::H5File handle(path, H5F_ACC_RDWR);
        auto ghandle = handle.openGroup("blub");
        auto hhandle = ghandle.openDataSet("heap");
        std::vector<uint8_t> replacement(heap.size());
        hhandle.write(replacement.data(), H5::PredType::NATIVE_UINT8);
    }
    {
        auto parsed = load_hdf5(path, "blub");
        EXPECT_EQ(parsed->type(), uzuki2::STRING);
        auto sptr = static_cast<const DefaultStringVector*>(parsed.get());
        EXPECT_EQ(sptr->size(), 1);
        EXPECT_EQ(sptr->base.values.front(), "");
    }

    // Checking that scalar works correctly with missing values.
    {
        H5::H5File handle(path, H5F_ACC_RDWR);
        auto ghandle = handle.openGroup("blub");
        auto dhandle = ghandle.openDataSet("data");
        H5::StrType stype(0, 10);
        auto ahandle = dhandle.createAttribute("missing-value-placeholder", stype, H5S_SCALAR);
        ahandle.write(stype, std::string{});
    }
    {
        auto parsed = load_hdf5(path, "blub");
        EXPECT_EQ(parsed->type(), uzuki2::STRING);
        auto sptr = static_cast<const DefaultStringVector*>(parsed.get());
        EXPECT_EQ(sptr->size(), 1);
        EXPECT_EQ(sptr->base.values.front(), "ich bin missing");
    }
}
