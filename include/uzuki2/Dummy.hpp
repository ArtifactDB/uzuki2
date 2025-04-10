#ifndef UZUKI2_DUMMY_HPP
#define UZUKI2_DUMMY_HPP

#include <vector>
#include <memory>
#include <string>
#include <cstdint>

#include "interfaces.hpp"

namespace uzuki2 {

/** Defining the simple vectors first. **/

class DummyIntegerVector final : public IntegerVector {
public:
    DummyIntegerVector(size_t l, bool, bool) : my_length(l) {}
    size_t size() const { return my_length; }
    void set(size_t, int32_t) {}
    void set_missing(size_t) {}
    void set_name(size_t, std::string) {}
private:
    size_t my_length;
};

class DummyNumberVector final : public NumberVector {
public:
    DummyNumberVector(size_t l, bool, bool) : my_length(l) {}
    size_t size() const { return my_length; }
    void set(size_t, double) {}
    void set_missing(size_t) {}
    void set_name(size_t, std::string) {}
private:
    size_t my_length;
};

class DummyStringVector final : public StringVector {
public:
    DummyStringVector(size_t l, bool, bool, StringVector::Format) : my_length(l) {}
    size_t size() const { return my_length; }
    void set(size_t, std::string) {}
    void set_missing(size_t) {}
    void set_name(size_t, std::string) {}
private:
    size_t my_length;
};

class DummyBooleanVector final : public BooleanVector {
public:
    DummyBooleanVector(size_t l, bool, bool) : my_length(l) {}
    size_t size() const { return my_length; }
    void set(size_t, bool) {}
    void set_missing(size_t) {}
    void set_name(size_t, std::string) {}
private:
    size_t my_length;
};

class DummyFactor final : public Factor {
public:
    DummyFactor(size_t l, bool, bool, size_t, bool) : my_length(l) {}
    size_t size() const { return my_length; }
    void set(size_t, size_t) {}
    void set_missing(size_t) {}
    void set_name(size_t, std::string) {}
    void set_level(size_t, std::string) {}
private:
    size_t my_length;
};

/** Defining the structural elements. **/

class DummyNothing final : public Nothing {};

class DummyExternal final : public External {};

class DummyList final : public List {
public:
    DummyList(size_t n, bool) : my_length(n) {}
    size_t size() const { return my_length; }
    void set(size_t, std::shared_ptr<Base>) {}
    void set_name(size_t, std::string) {}
private:
    size_t my_length;
};

/** Dummy provisioner. **/

struct DummyProvisioner {
    static Nothing* new_Nothing() { return (new DummyNothing); }

    static External* new_External(void*) { return (new DummyExternal); }

    template<class ... Args_>
    static List* new_List(Args_&& ... args) { return (new DummyList(std::forward<Args_>(args)...)); }

    template<class ... Args_>
    static IntegerVector* new_Integer(Args_&& ... args) { return (new DummyIntegerVector(std::forward<Args_>(args)...)); }

    template<class ... Args_>
    static NumberVector* new_Number(Args_&& ... args) { return (new DummyNumberVector(std::forward<Args_>(args)...)); }

    template<class ... Args_>
    static StringVector* new_String(Args_&& ... args) { return (new DummyStringVector(std::forward<Args_>(args)...)); }

    template<class ... Args_>
    static BooleanVector* new_Boolean(Args_&& ... args) { return (new DummyBooleanVector(std::forward<Args_>(args)...)); }

    template<class ... Args_>
    static Factor* new_Factor(Args_&& ... args) { return (new DummyFactor(std::forward<Args_>(args)...)); }
};

class DummyExternals {
public:
    DummyExternals(size_t n) : my_number(n) {}

    void* get(size_t) const {
        return nullptr;
    }

    size_t size() const {
        return my_number;
    }

private:
    size_t my_number;
};

}

#endif
