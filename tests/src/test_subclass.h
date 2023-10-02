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
    DefaultVectorBase(size_t n) : values(n) {}

    size_t size() const { 
        return values.size(); 
    }

    void set(size_t i, T val) {
        values[i] = std::move(val);
        return;
    }

    void use_names() {
        has_names = true;
        names.resize(values.size());
        return;
    }

    void set_missing(size_t i) {
        set_missing_internal(values, i);
        return;
    }

    void set_name(size_t i, std::string name) {
        names[i] = std::move(name);
        return;
    }

    std::vector<T> values;
    bool has_names = false;
    std::vector<std::string> names;
};

struct DefaultIntegerVector : public uzuki2::IntegerVector {
    DefaultIntegerVector(size_t n) : base(n) {}

    size_t size() const { 
        return base.size();
    }

    void set(size_t i, int32_t val) {
        base.set(i, val);
        return;
    }

    void use_names() {
        base.use_names();
        return;
    }

    void set_missing(size_t i) {
        base.set_missing(i);
        return;
    }

    void set_name(size_t i, std::string name) {
        base.set_name(i, std::move(name));
        return;
    }

    void is_scalar() {
        scalar = true;
    }

    DefaultVectorBase<int32_t> base;
    bool scalar = false;
};

struct DefaultNumberVector : public uzuki2::NumberVector {
    DefaultNumberVector(size_t n) : base(n) {}

    size_t size() const { 
        return base.size();
    }

    void set(size_t i, double val) {
        base.set(i, val);
        return;
    }

    void use_names() {
        base.use_names();
        return;
    }

    void set_missing(size_t i) {
        base.set_missing(i);
        return;
    }

    void set_name(size_t i, std::string name) {
        base.set_name(i, std::move(name));
        return;
    }

    void is_scalar() {
        scalar = true;
    }

    DefaultVectorBase<double> base;
    bool scalar = false;
};

struct DefaultBooleanVector : public uzuki2::BooleanVector {
    DefaultBooleanVector(size_t n) : base(n) {}

    size_t size() const { 
        return base.size();
    }

    void set(size_t i, bool val) {
        base.set(i, val);
        return;
    }

    void use_names() {
        base.use_names();
        return;
    }

    void set_missing(size_t i) {
        base.set_missing(i);
        return;
    }

    void set_name(size_t i, std::string name) {
        base.set_name(i, std::move(name));
        return;
    }

    void is_scalar() {
        scalar = true;
    }

    DefaultVectorBase<uint8_t> base;
    bool scalar = false;
};

struct DefaultStringVector : public uzuki2::StringVector {
    DefaultStringVector(size_t n, uzuki2::StringVector::Format f) : base(n), format(f) {}

    size_t size() const { 
        return base.size();
    }

    void set(size_t i, std::string val) {
        base.set(i, std::move(val));
        return;
    }

    void use_names() {
        base.use_names();
        return;
    }

    void set_missing(size_t i) {
        base.set_missing(i);
        return;
    }

    void set_name(size_t i, std::string name) {
        base.set_name(i, std::move(name));
        return;
    }

    void is_scalar() {
        scalar = true;
    }

    DefaultVectorBase<std::string> base;
    StringVector::Format format;
    bool scalar = false;
};

struct DefaultFactor : public uzuki2::Factor {
    DefaultFactor(size_t l, size_t ll) : vbase(l), levels(ll) {}

    size_t size() const { 
        return vbase.size(); 
    }

    void set(size_t i, size_t l) {
        vbase.set(i, l);
        return;
    }

    void use_names() {
        vbase.use_names();
        return;
    }

    void set_missing(size_t i) {
        vbase.set_missing(i);
        return;
    }

    void set_name(size_t i, std::string name) {
        vbase.set_name(i, std::move(name));
        return;
    }

    void set_level(size_t i, std::string l) {
        levels[i] = std::move(l);
        return;
    }

    void is_ordered() {
        ordered = true;
        return;
    }

    DefaultVectorBase<size_t> vbase;
    std::vector<std::string> levels;
    bool ordered = false;
};

/** Defining the structural elements. **/

struct DefaultNothing : public uzuki2::Nothing {};

struct DefaultExternal : public uzuki2::External {
    DefaultExternal(void *p) : ptr(p) {}
    void* ptr;
};

struct DefaultList : public uzuki2::List {
    DefaultList(size_t n) : values(n) {}

    size_t size() const { 
        return values.size(); 
    }

    void set(size_t i, std::shared_ptr<uzuki2::Base> ptr) {
        values[i] = std::move(ptr);
        return;
    }

    void set_name(size_t i, std::string name) {
        names[i] = std::move(name);
        return;
    }

    void use_names() {
        has_names = true;
        names.resize(values.size());
        return;
    }

    std::vector<std::shared_ptr<uzuki2::Base> > values;
    bool has_names = false;
    std::vector<std::string> names;
};

/** Provisioner. **/

struct DefaultProvisioner {
    static uzuki2::Nothing* new_Nothing() { return (new DefaultNothing); }

    static uzuki2::External* new_External(void* p) { return (new DefaultExternal(p)); }

    static uzuki2::List* new_List(size_t l) { return (new DefaultList(l)); }

    static uzuki2::IntegerVector* new_Integer(size_t l) { return (new DefaultIntegerVector(l)); }

    static uzuki2::NumberVector* new_Number(size_t l) { return (new DefaultNumberVector(l)); }

    static uzuki2::StringVector* new_String(size_t l, uzuki2::StringVector::Format f) { return (new DefaultStringVector(l, f)); }

    static uzuki2::BooleanVector* new_Boolean(size_t l) { return (new DefaultBooleanVector(l)); }

    static uzuki2::Factor* new_Factor(size_t l, size_t ll) { return (new DefaultFactor(l, ll)); }
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
