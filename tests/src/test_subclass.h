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

template<typename T, uzuki2::Type tt>
struct DefaultTypedVector : public uzuki2::TypedVector<T, tt> {
    DefaultTypedVector(size_t n) : base(n) {}

    size_t size() const { 
        return base.size();
    }

    void set(size_t i, T val) {
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

    DefaultVectorBase<T> base;
    bool scalar = false;
};

typedef DefaultTypedVector<int32_t, uzuki2::INTEGER> DefaultIntegerVector; 
typedef DefaultTypedVector<double, uzuki2::NUMBER> DefaultNumberVector;
typedef DefaultTypedVector<std::string, uzuki2::STRING> DefaultStringVector;
typedef DefaultTypedVector<unsigned char, uzuki2::BOOLEAN> DefaultBooleanVector;
typedef DefaultTypedVector<std::string, uzuki2::DATE> DefaultDateVector;

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

    static uzuki2::StringVector* new_String(size_t l) { return (new DefaultStringVector(l)); }

    static uzuki2::BooleanVector* new_Boolean(size_t l) { return (new DefaultBooleanVector(l)); }

    static uzuki2::DateVector* new_Date(size_t l) { return (new DefaultDateVector(l)); }

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
