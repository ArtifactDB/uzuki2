#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "uzuki2/parse_hdf5.hpp"

#include "utils.h"

H5::DataSet dump_heap(H5::Group& handle, const char* heap, const hsize_t hlen) {
    H5::DataSpace vspace(1, &hlen);
    auto vhandle = handle.createDataSet("heap", H5::PredType::NATIVE_UINT8, vspace);
    std::vector<std::uint8_t> buffer(hlen);
    std::copy_n(heap, hlen, reinterpret_cast<char*>(buffer.data()));
    vhandle.write(buffer.data(), H5::PredType::NATIVE_UINT8);
    return vhandle;
}

template<typename Offset_, typename Length_>
H5::DataSet dump_pointers(H5::Group& handle, const std::vector<ritsuko::cvls::Pointer<Offset_, Length_> >& pointers) {
    const hsize_t plen = pointers.size();
    H5::DataSpace pspace(1, &plen);
    const auto ptype = ritsuko::cvls::define_pointer_datatype<Offset_, Length_>();
    auto phandle = handle.createDataSet("data", ptype, pspace);
    phandle.write(pointers.data(), ptype);
    return phandle;
}

TEST(Hdf5Vls, Vector) {
    auto path = "TEST-vls.h5";
    std::string heap = "abcdefghijklmno";
    hsize_t nlen = 10;

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "vls");
        add_version(vhandle, "1.4");
        dump_heap(vhandle, heap.c_str(), heap.size());

        std::vector<ritsuko::cvls::Pointer<uint64_t, uint64_t> > pointers(nlen);
        std::size_t n = 0;
        for (hsize_t i = 0; i < nlen; ++i) {
            pointers[i].offset = n;
            std::size_t count = (i % 2) + 1; // for some interesting differences.
            pointers[i].length = count;
            n += count;
        }
        dump_pointers(vhandle, pointers);
    }

    auto parsed = load_hdf5(path, "blub");
    EXPECT_EQ(parsed->type(), uzuki2::STRING);
    auto sptr = static_cast<const DefaultStringVector*>(parsed.get());
    EXPECT_EQ(sptr->size(), nlen);
    std::vector<std::string> expected { "a", "bc", "d", "ef", "g", "hi", "j", "kl", "m", "no" };
    EXPECT_EQ(sptr->base.values, expected);
}

TEST(Hdf5Vls, Scalar) {
    auto path = "TEST-vls.h5";
    std::string heap = "abcdefghijklmno";

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "vls");
        add_version(vhandle, "1.4");
        dump_heap(vhandle, heap.c_str(), heap.size());

        ritsuko::cvls::Pointer<uint8_t, uint8_t> ptr;
        ptr.offset = 5;
        ptr.length = 10;
        auto ptype = ritsuko::cvls::define_pointer_datatype<std::uint8_t, std::uint8_t>();
        auto phandle = vhandle.createDataSet("data", ptype, H5S_SCALAR);
        phandle.write(&ptr, ptype);
    }

    auto parsed = load_hdf5(path, "blub");
    EXPECT_EQ(parsed->type(), uzuki2::STRING);
    auto sptr = static_cast<const DefaultStringVector*>(parsed.get());
    EXPECT_EQ(sptr->size(), 1);
    EXPECT_TRUE(sptr->base.scalar);
    EXPECT_EQ(sptr->base.values[0], "fghijklmno");
}

TEST(Hdf5Vls, ScalarNullTerminated) {
    auto path = "TEST-vls.h5";
    std::vector<char> heap(20);
    heap[5] = 'f';
    heap[6] = 'g';
    heap[8] = 'i';

    // Check for correct early truncation by injecting some nulls.
    // We only do this for scalars because it's handled by Stream1dArray for vectors.
    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "vls");
        add_version(vhandle, "1.4");
        dump_heap(vhandle, heap.data(), heap.size());

        ritsuko::cvls::Pointer<uint8_t, uint8_t> ptr;
        ptr.offset = 5;
        ptr.length = 10;
        auto ptype = ritsuko::cvls::define_pointer_datatype<uint8_t, uint8_t>();
        auto phandle = vhandle.createDataSet("data", ptype, H5S_SCALAR);
        phandle.write(&ptr, ptype);
    }

    auto parsed = load_hdf5(path, "blub");
    EXPECT_EQ(parsed->type(), uzuki2::STRING);
    auto sptr = static_cast<const DefaultStringVector*>(parsed.get());
    EXPECT_EQ(sptr->size(), 1);
    EXPECT_EQ(sptr->base.values[0], "fg");
}

TEST(Hdf5Vls, ForbiddenPointerType) {
    auto path = "TEST-vls.h5";
    std::string heap = "abcdefghijklmno";

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "vls");
        add_version(vhandle, "1.4");
        dump_heap(vhandle, heap.c_str(), heap.size());

        struct Foo {
            int offset, length;
        };
        H5::CompType ptype(sizeof(Foo));
        ptype.insertMember("offset", HOFFSET(Foo, offset), H5::PredType::NATIVE_INT);
        ptype.insertMember("length", HOFFSET(Foo, length), H5::PredType::NATIVE_INT);
        const hsize_t nlen = 10;
        vhandle.createDataSet("data", ptype, H5::DataSpace(1, &nlen));
    }
    expect_hdf5_error(path, "blub", "64-bit unsigned integer");
}

TEST(Hdf5Vls, HeapError) {
    auto path = "TEST-vls.h5";
    std::string heap = "abcdefghijklmno";

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto ghandle = vector_opener(handle, "blub", "vls");
        add_version(ghandle, "1.4");
        std::vector<ritsuko::cvls::Pointer<std::uint64_t, std::uint64_t> > pointers(10);
        dump_pointers(ghandle, pointers);

        const hsize_t hlen = heap.size();
        H5::DataSpace hspace(1, &hlen);
        ghandle.createDataSet("heap", H5::PredType::NATIVE_INT8, hspace);
    }
    expect_hdf5_error(path, "blub", "8-bit unsigned integer");
}

TEST(Hdf5Vls, ScalarOutOfRange) {
    auto path = "TEST-vls.h5";
    std::string heap = "abcdefghijklmno";

    // Check for scalar pointers being out of range.
    // We only do this for scalars because it's handled by Stream1dArray for vectors.
    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "vls");
        add_version(vhandle, "1.4");
        dump_heap(vhandle, heap.c_str(), heap.size());

        ritsuko::cvls::Pointer<uint8_t, uint8_t> ptr;
        ptr.offset = 0;
        ptr.length = 100;
        auto ptype = ritsuko::cvls::define_pointer_datatype<std::uint8_t, std::uint8_t>();
        auto phandle = vhandle.createDataSet("data", ptype, H5S_SCALAR);
        phandle.write(&ptr, ptype);
    }
    expect_hdf5_error(path, "blub", "out of range");
}

TEST(Hdf5Vls, MissingPlaceholderVector) {
    auto path = "TEST-vls.h5";
    std::string heap = "abcdefghijklmno";
    hsize_t nlen = 10;

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "vls");
        add_version(vhandle, "1.4");
        dump_heap(vhandle, heap.c_str(), heap.size());

        std::vector<ritsuko::cvls::Pointer<uint64_t, uint64_t> > pointers(nlen);
        std::size_t n = 0;
        for (hsize_t i = 0; i < nlen; ++i) {
            pointers[i].offset = n;
            std::size_t count = (i % 2) + 1; // for some interesting differences.
            pointers[i].length = count;
            n += count;
        }
        auto dhandle = dump_pointers(vhandle, pointers);

        H5::StrType stype(0, H5T_VARIABLE);
        auto ahandle = dhandle.createAttribute("missing-value-placeholder", stype, H5S_SCALAR);
        ahandle.write(stype, std::string("hi"));
    }

    auto parsed = load_hdf5(path, "blub");
    EXPECT_EQ(parsed->type(), uzuki2::STRING);
    auto sptr = static_cast<const DefaultStringVector*>(parsed.get());
    EXPECT_EQ(sptr->base.values[5], "ich bin missing"); // the test's missing placeholder.
}

TEST(Hdf5Vls, MissingPlaceholderScalar) {
    auto path = "TEST-vls.h5";
    std::string heap = "abcdefghijklmno";

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "vls");
        add_version(vhandle, "1.4");
        dump_heap(vhandle, heap.c_str(), heap.size());

        ritsuko::cvls::Pointer<uint8_t, uint8_t> ptr;
        ptr.offset = 5;
        ptr.length = 10;
        auto ptype = ritsuko::cvls::define_pointer_datatype<std::uint8_t, std::uint8_t>();
        auto phandle = vhandle.createDataSet("data", ptype, H5S_SCALAR);
        phandle.write(&ptr, ptype);

        H5::StrType stype(0, 10); // using a fixed placeholder, for some variety.
        auto ahandle = phandle.createAttribute("missing-value-placeholder", stype, H5S_SCALAR);
        ahandle.write(stype, std::string("fghijklmno"));
    }

    auto parsed = load_hdf5(path, "blub");
    EXPECT_EQ(parsed->type(), uzuki2::STRING);
    auto sptr = static_cast<const DefaultStringVector*>(parsed.get());
    EXPECT_EQ(sptr->size(), 1);
    EXPECT_EQ(sptr->base.values[0], "ich bin missing"); // the test's missing placeholder.
}

TEST(Hdf5Vls, MissingPlaceholderError) {
    auto path = "TEST-vls.h5";
    std::string heap = "abcdefghijklmno";

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "blub", "vls");
        add_version(vhandle, "1.4");
        dump_heap(vhandle, heap.c_str(), heap.size());

        std::vector<ritsuko::cvls::Pointer<uint64_t, uint64_t> > pointers(10);
        auto phandle = dump_pointers(vhandle, pointers);
        constexpr hsize_t one = 1;
        phandle.createAttribute("missing-value-placeholder", H5::StrType(0, H5T_VARIABLE), H5::DataSpace(1, &one));
    }
    expect_hdf5_error(path, "blub", "scalar");

    {
        H5::H5File handle(path, H5F_ACC_RDWR);
        auto vhandle = handle.openDataSet("blub/data");
        vhandle.removeAttr("missing-value-placeholder");
        vhandle.createAttribute("missing-value-placeholder", H5::PredType::NATIVE_INT, H5S_SCALAR);
    }
    expect_hdf5_error(path, "blub", "attribute to be a UTF-8 string");
}
