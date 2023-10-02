#ifndef UZUKI2_DUMMY_HPP
#define UZUKI2_DUMMY_HPP

#include <vector>
#include <memory>
#include <string>
#include <cstdint>

#include "interfaces.hpp"

namespace uzuki2 {

/** Defining the simple vectors first. **/

struct DummyIntegerVector : public IntegerVector {
    DummyIntegerVector(size_t l) : length(l) {}

    size_t size() const { return length; }
    size_t length;
    void is_scalar() {}

    void set(size_t, int32_t) {}
    void set_missing(size_t) {}
    void set_name(size_t, std::string) {}
    void use_names() {}
};

struct DummyNumberVector : public NumberVector {
    DummyNumberVector(size_t l) : length(l) {}

    size_t size() const { return length; }
    size_t length;
    void is_scalar() {}

    void set(size_t, double) {}
    void set_missing(size_t) {}
    void set_name(size_t, std::string) {}
    void use_names() {}
};

struct DummyStringVector : public StringVector {
    DummyStringVector(size_t l, StringVector::Format) : length(l) {}

    size_t size() const { return length; }
    size_t length;
    void is_scalar() {}

    void set(size_t, std::string) {}
    void set_missing(size_t) {}
    void set_name(size_t, std::string) {}
    void use_names() {}
};

struct DummyBooleanVector : public BooleanVector {
    DummyBooleanVector(size_t l) : length(l) {}

    size_t size() const { return length; }
    size_t length;
    void is_scalar() {}

    void set(size_t, bool) {}
    void set_missing(size_t) {}
    void set_name(size_t, std::string) {}
    void use_names() {}
};

struct DummyFactor : public Factor {
    DummyFactor(size_t l, size_t) : length(l) {}

    size_t size() const { return length; }
    size_t length;
    void is_scalar() {}

    void set(size_t, size_t) {}
    void set_missing(size_t) {}
    void set_name(size_t, std::string) {}
    void use_names() {}

    void set_level(size_t, std::string) {}
    void is_ordered() {}
};

/** Defining the structural elements. **/

struct DummyNothing : public Nothing {};

struct DummyExternal : public External {};

struct DummyList : public List {
    DummyList(size_t n) : length(n) {}

    size_t size() const { return length; }
    size_t length;

    void set(size_t, std::shared_ptr<Base>) {}
    void set_name(size_t, std::string) {}
    void use_names() {}
};

/** Dummy provisioner. **/

struct DummyProvisioner {
    static Nothing* new_Nothing() { return (new DummyNothing); }

    static External* new_External(void*) { return (new DummyExternal); }

    static List* new_List(size_t l) { return (new DummyList(l)); }

    static IntegerVector* new_Integer(size_t l) { return (new DummyIntegerVector(l)); }

    static NumberVector* new_Number(size_t l) { return (new DummyNumberVector(l)); }

    static StringVector* new_String(size_t l, StringVector::Format f) { return (new DummyStringVector(l, f)); }

    static BooleanVector* new_Boolean(size_t l) { return (new DummyBooleanVector(l)); }

    static Factor* new_Factor(size_t l, size_t ll) { return (new DummyFactor(l, ll)); }
};

struct DummyExternals {
    DummyExternals(size_t n) : number(n) {}

    void* get(size_t) const {
        return nullptr;
    }

    size_t size() const {
        return number;
    }

    size_t number;
};

}

#endif
