#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <optional>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "uzuki2/parse_hdf5.hpp"

#include "test_subclass.h"
#include "utils.h"

TEST(Hdf5Attribute, CheckError) {
    auto path = "TEST-date.h5";

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        handle.createGroup("whee");
    }
    expect_hdf5_error(path, "whee", "uzuki_object");

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        std::map<std::string, std::string> attrs;
        attrs["uzuki_object"] = "blah";
        super_group_opener(handle, "whee", attrs);
    }
    expect_hdf5_error(path, "whee", "unknown uzuki2 object");

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        std::map<std::string, std::string> attrs;
        attrs["uzuki_object"] = "vector";
        super_group_opener(handle, "whee", attrs);
    }
    expect_hdf5_error(path, "whee", "uzuki_type");

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto vhandle = vector_opener(handle, "whee", "BLAH");
        create_dataset<int>(vhandle, "data", { 1, 2, 3, 4, 5 }, H5::PredType::NATIVE_INT);
    }
    expect_hdf5_error(path, "whee", "unknown vector type");
}

TEST(TopLevelList, CheckErrors) {
    auto path = "TEST-date.h5";

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        std::map<std::string, std::string> attrs;
        attrs["uzuki_object"] = "nothing";
        super_group_opener(handle, "whee", attrs);
    }
    expect_hdf5_error(path, "whee", "top-level object should represent an R list");

    expect_json_error("{ \"type\":\"nothing\" }", "top-level object should represent an R list");
}

TEST(Version, Error) {
    expect_json_error("{ version: true }", "expected a string");
}

class ParseOverloadTest : public ::testing::TestWithParam<std::tuple<int, bool, std::pair<bool, int> > > {
protected:
    static std::string dump_file(const std::string& payload, int mode) {
        std::string path = "TEST.json";
        std::unique_ptr<byteme::Writer> writer;
        if (mode == 0) {
            writer.reset(new byteme::RawFileWriter(path.c_str(), {}));
        } else {
            path += ".gz";
            writer.reset(new byteme::GzipFileWriter(path.c_str(), {}));
        }
        writer->write(reinterpret_cast<const unsigned char*>(payload.c_str()), payload.size());
        return path;
    }

    static std::vector<unsigned char> dump_buffer(const std::string& payload, int mode) {
        if (mode == 0) {
            byteme::RawBufferWriter writer({});
            writer.write(reinterpret_cast<const unsigned char*>(payload.c_str()), payload.size());
            writer.finish();
            return std::move(writer.get_output());
        } else {
            byteme::ZlibBufferWriterOptions opts;
            opts.mode = (mode == 1 ? byteme::ZlibCompressionMode::ZLIB : byteme::ZlibCompressionMode::GZIP);
            byteme::ZlibBufferWriter writer(opts);
            writer.write(reinterpret_cast<const unsigned char*>(payload.c_str()), payload.size());
            writer.finish();
            return std::move(writer.get_output());
        }
    }
};

TEST_P(ParseOverloadTest, Basic) {
    auto param = GetParam();
    uzuki2::json::Options opt;
    opt.buffer_size = std::get<0>(param);
    opt.parallel = std::get<1>(param);
    bool use_buffer = std::get<2>(param).first;
    int compression = std::get<2>(param).second;

    {
        std::optional<uzuki2::ParsedList> parsed;
        std::string payload = "{ \"type\":\"list\",\n  \"values\": [ { \"type\": \"nothing\" }, { \"type\": \"integer\", \"values\": [ 1, 2, 3 ] } ], \n\t\"names\": [ \"X\", \"Y\" ] }";

        if (use_buffer) {
            auto buffered = dump_buffer(payload, compression);
            parsed = uzuki2::json::parse_buffer<DefaultProvisioner>(buffered.data(), buffered.size(), uzuki2::DummyExternals(0), opt);
            EXPECT_NO_THROW(uzuki2::json::validate_buffer(buffered.data(), buffered.size(), 0, {}));
        } else {
            auto path = dump_file(payload, compression);
            parsed = uzuki2::json::parse_file<DefaultProvisioner>(path, uzuki2::DummyExternals(0), opt);
            EXPECT_NO_THROW(uzuki2::json::validate_file(path, 0, {}));
        }

        EXPECT_EQ(parsed->ptr->type(), uzuki2::LIST);

        auto stuff = static_cast<const DefaultList*>(parsed->ptr.get());
        EXPECT_EQ(stuff->size(), 2);

        EXPECT_EQ(stuff->values[0]->type(), uzuki2::NOTHING);
        EXPECT_EQ(stuff->values[1]->type(), uzuki2::INTEGER);

        auto stuff2 = static_cast<const DefaultIntegerVector*>(stuff->values[1].get());
        EXPECT_EQ(stuff2->base.values[0], 1);
        EXPECT_EQ(stuff2->base.values[2], 3);

        EXPECT_TRUE(stuff->has_names);
        EXPECT_EQ(stuff->names[0], "X");
        EXPECT_EQ(stuff->names[1], "Y");
    }

    // Again, with some externals.
    {
        std::optional<uzuki2::ParsedList> parsed;
        std::string payload = "{ \"type\":\"list\",\n  \"values\": [ { \"type\": \"external\", \"index\": 0 }, { \"type\": \"external\", \"index\": 1 } ] }";

        if (use_buffer) {
            auto buffered = dump_buffer(payload, compression);
            parsed = uzuki2::json::parse_buffer<DefaultProvisioner>(buffered.data(), buffered.size(), DefaultExternals(2), opt);
            EXPECT_NO_THROW(uzuki2::json::validate_buffer(buffered.data(), buffered.size(), 2, {}));
        } else {
            auto path = dump_file(payload, compression);
            parsed = uzuki2::json::parse_file<DefaultProvisioner>(path, DefaultExternals(2), opt);
            EXPECT_NO_THROW(uzuki2::json::validate_file(path, 2, {}));
        }

        EXPECT_EQ(parsed->ptr->type(), uzuki2::LIST);
    }
}

INSTANTIATE_TEST_SUITE_P(
    Parse,
    ParseOverloadTest,
    ::testing::Combine(
        ::testing::Values(3, 7, 13, 17, 29), // block size per read.
        ::testing::Values(false, true), // whether or not it's parallelized.
        ::testing::Values(
            std::make_pair(false, 0), // file, uncompressed
            std::make_pair(false, 1), // file, gzip-compressed
            std::make_pair(true, 0), // buffer, uncompressed
            std::make_pair(true, 1), // buffer, zlib-compressed
            std::make_pair(true, 2)  // buffer, gzip-compressed
        )
    )
);

