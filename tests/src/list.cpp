#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "uzuki2/parse.hpp"
#include "uzuki2/Dummy.hpp"
#include "test_subclass.h"
#include "utils.h"

TEST(ListTest, SimpleLoading) {
    auto path = "TEST-list.h5";

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto ghandle = list_opener(handle, "foo");
        auto dhandle = ghandle.createGroup("data");
        null_opener(dhandle, "0");
        null_opener(dhandle, "1");
    }
    {
        uzuki2::DummyExternals ext(-1);
        auto parsed = uzuki2::parse<DefaultProvisioner>(path, ext, "foo");
        EXPECT_EQ(parsed->type(), uzuki2::LIST);

        auto stuff = static_cast<const DefaultList*>(parsed.get());
        EXPECT_EQ(stuff->size(), 2);
    }



}
