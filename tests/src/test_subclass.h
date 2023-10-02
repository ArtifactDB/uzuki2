#ifndef TEST_SUBCLASSES_H 
#define TEST_SUBCLASSES_H 

#include <vector>
#include <string>
#include <memory>
#include <cstdint>
#include <limits>

#include "uzuki2/interfaces.hpp"

template<typename T>
void set_missing_internal(std::vector<T>& values, size_t i) {
    if constexpr(std::is_same<T, double>::value) {
        values[i] = -123456789;
    }
    if constexpr(std::is_same<T, size_t>::value) {
        values[i] = -1;
    }
    if constexpr(std::is_same<T, unsigned char>::value) {
        values[i] = -1;
    }
    if constexpr(std::is_same<T, int32_t>::value) {
        values[i] = -123456789;
    }
    if constexpr(std::is_same<T, std::string>::value) {
        values[i] = "ich bin missing";
    }
}

/** Defining the simple vectors first. **/

template<typename T>
struct DefaultVectorBase { 
    DefaultVectorBase(size_t l, bool n, bool s) : values(l), has_names(n), names(n ? l : 0), scalar(s) {}

    size_t size() const { 
        return values.size(); 
    }

    void set(size_t i, T val) {
        values[i] = std::move(val);
    }

    void set_missing(size_t i) {
        set_missing_internal(values, i);
    }

    void set_name(size_t i, std::string name) {
        names[i] = std::move(name);
    }

    std::vector<T> values;
    bool has_names;
    std::vector<std::string> names;
    bool scalar;
};

struct DefaultIntegerVector : public uzuki2::IntegerVector {
    DefaultIntegerVector(size_t l, bool n, bool s) : base(l, n, s) {}

    size_t size() const { 
        return base.size();
    }

    void set(size_t i, int32_t val) {
        base.set(i, val);
    }

    void set_missing(size_t i) {
        base.set_missing(i);
    }

    void set_name(size_t i, std::string name) {
        base.set_name(i, std::move(name));
    }

    DefaultVectorBase<int32_t> base;
};

struct DefaultNumberVector : public uzuki2::NumberVector {
    DefaultNumberVector(size_t l, bool n, bool s) : base(l, n, s) {}

    size_t size() const { 
        return base.size();
    }

    void set(size_t i, double val) {
        base.set(i, val);
    }

    void set_missing(size_t i) {
        base.set_missing(i);
    }

    void set_name(size_t i, std::string name) {
        base.set_name(i, std::move(name));
    }

    DefaultVectorBase<double> base;
};

struct DefaultBooleanVector : public uzuki2::BooleanVector {
    DefaultBooleanVector(size_t l, bool n, bool s) : base(l, n, s) {}

    size_t size() const { 
        return base.size();
    }

    void set(size_t i, bool val) {
        base.set(i, val);
    }

    void set_missing(size_t i) {
        base.set_missing(i);
    }

    void set_name(size_t i, std::string name) {
        base.set_name(i, std::move(name));
    }

    DefaultVectorBase<uint8_t> base;
};

struct DefaultStringVector : public uzuki2::StringVector {
    DefaultStringVector(size_t l, bool n, bool s, uzuki2::StringVector::Format f) : base(l, n, s), format(f) {}

    size_t size() const { 
        return base.size();
    }

    void set(size_t i, std::string val) {
        base.set(i, std::move(val));
    }

    void set_missing(size_t i) {
        base.set_missing(i);
    }

    void set_name(size_t i, std::string name) {
        base.set_name(i, std::move(name));
    }

    DefaultVectorBase<std::string> base;
    StringVector::Format format;
};

struct DefaultFactor : public uzuki2::Factor {
    DefaultFactor(size_t l, bool n, bool s, size_t ll, bool o) : vbase(l, n, s), levels(ll), ordered(o) {}

    size_t size() const { 
        return vbase.size(); 
    }

    void set(size_t i, size_t l) {
        vbase.set(i, l);
    }

    void set_missing(size_t i) {
        vbase.set_missing(i);
    }

    void set_name(size_t i, std::string name) {
        vbase.set_name(i, std::move(name));
    }

    void set_level(size_t i, std::string l) {
        levels[i] = std::move(l);
    }

    DefaultVectorBase<size_t> vbase;
    std::vector<std::string> levels;
    bool ordered;
};

/** Defining the structural elements. **/

struct DefaultNothing : public uzuki2::Nothing {};

struct DefaultExternal : public uzuki2::External {
    DefaultExternal(void *p) : ptr(p) {}
    void* ptr;
};

struct DefaultList : public uzuki2::List {
    DefaultList(size_t l, bool n) : values(l), has_names(n), names(n ? l : 0) {}

    size_t size() const { 
        return values.size(); 
    }

    void set(size_t i, std::shared_ptr<uzuki2::Base> ptr) {
        values[i] = std::move(ptr);
    }

    void set_name(size_t i, std::string name) {
        names[i] = std::move(name);
    }

    std::vector<std::shared_ptr<uzuki2::Base> > values;
    bool has_names = false;
    std::vector<std::string> names;
};

/** Provisioner. **/

struct DefaultProvisioner {
    static uzuki2::Nothing* new_Nothing() { return (new DefaultNothing); }

    static uzuki2::External* new_External(void* p) { return (new DefaultExternal(p)); }

    template<class ... Args_>
    static uzuki2::List* new_List(Args_&& ... args) { return (new DefaultList(std::forward<Args_>(args)...)); }

    template<class ... Args_>
    static uzuki2::IntegerVector* new_Integer(Args_&& ... args) { return (new DefaultIntegerVector(std::forward<Args_>(args)...)); }

    template<class ... Args_>
    static uzuki2::NumberVector* new_Number(Args_&& ... args) { return (new DefaultNumberVector(std::forward<Args_>(args)...)); }

    template<class ... Args_>
    static uzuki2::StringVector* new_String(Args_&& ... args) { return (new DefaultStringVector(std::forward<Args_>(args)...)); }

    template<class ... Args_>
    static uzuki2::BooleanVector* new_Boolean(Args_&& ... args) { return (new DefaultBooleanVector(std::forward<Args_>(args)...)); }

    template<class ... Args_>
    static uzuki2::Factor* new_Factor(Args_&& ... args) { return (new DefaultFactor(std::forward<Args_>(args)...)); }
};

struct DefaultExternals {
    DefaultExternals(size_t n) : number(n) {}

    void* get(size_t i) {
        return reinterpret_cast<void*>(static_cast<uintptr_t>(i + 1));
    }

    size_t size() const {
        return number;
    }

    size_t number;
};

#endif
