#ifndef UZUKI2_DUMMY_HPP
#define UZUKI2_DUMMY_HPP

#include <vector>
#include <memory>
#include <string>
#include <cstdint>
#include <cstddef>

#include "interfaces.hpp"

/**
 * @file Dummy.hpp
 * @brief Dummy classes for parsing without storing the results.
 */

namespace uzuki2 {

/**
 * @cond
 */

class DummyIntegerVector final : public IntegerVector {
public:
    DummyIntegerVector(std::size_t l, bool, bool) : my_length(l) {}
    std::size_t size() const { return my_length; }
    void set(std::size_t, std::int32_t) {}
    void set_missing(std::size_t) {}
    void set_name(std::size_t, std::string) {}
private:
    std::size_t my_length;
};

class DummyNumberVector final : public NumberVector {
public:
    DummyNumberVector(std::size_t l, bool, bool) : my_length(l) {}
    std::size_t size() const { return my_length; }
    void set(std::size_t, double) {}
    void set_missing(std::size_t) {}
    void set_name(std::size_t, std::string) {}
private:
    std::size_t my_length;
};

class DummyStringVector final : public StringVector {
public:
    DummyStringVector(std::size_t l, bool, bool, StringVector::Format) : my_length(l) {}
    std::size_t size() const { return my_length; }
    void set(std::size_t, std::string) {}
    void set_missing(std::size_t) {}
    void set_name(std::size_t, std::string) {}
private:
    std::size_t my_length;
};

class DummyBooleanVector final : public BooleanVector {
public:
    DummyBooleanVector(std::size_t l, bool, bool) : my_length(l) {}
    std::size_t size() const { return my_length; }
    void set(std::size_t, bool) {}
    void set_missing(std::size_t) {}
    void set_name(std::size_t, std::string) {}
private:
    std::size_t my_length;
};

class DummyFactor final : public Factor {
public:
    DummyFactor(std::size_t l, bool, bool, std::size_t, bool) : my_length(l) {}
    std::size_t size() const { return my_length; }
    void set(std::size_t, std::size_t) {}
    void set_missing(std::size_t) {}
    void set_name(std::size_t, std::string) {}
    void set_level(std::size_t, std::string) {}
private:
    std::size_t my_length;
};

/** Defining the structural elements. **/

class DummyNothing final : public Nothing {};

class DummyExternal final : public External {};

class DummyList final : public List {
public:
    DummyList(std::size_t n, bool) : my_length(n) {}
    std::size_t size() const { return my_length; }
    void set(std::size_t, std::shared_ptr<Base>) {}
    void set_name(std::size_t, std::string) {}
private:
    std::size_t my_length;
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
/**
 * @endcond
 */

/**
 * @brief Dummy class satisfying the `Externals_` interface of `hdf5::parse()`.
 *
 * Users should only create an instance of this class with the default constructor.
 * This can then be used as the `ext` argument in `json::parse()` and `hdf5::parse()`.
 * Doing so will indicate that no external references are expected during parsing.
 */
class DummyExternals {
public:
    DummyExternals() : DummyExternals(0) {}

private:
    std::size_t my_number;

public:
    /**
     * @cond
     */
    DummyExternals(std::size_t n) : my_number(n) {}

    void* get(std::size_t) const {
        return nullptr;
    }

    std::size_t size() const {
        return my_number;
    }
    /**
     * @endcond
     */
};

}

#endif
