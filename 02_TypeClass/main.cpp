#include <string>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <format>
#include <memory>
#include <variant>
#include <type_traits>
#include <optional>
#include <cassert>

#include <iostream>

struct Type;

/// <summary>
/// 型への参照
/// </summary>
using RefType = std::shared_ptr<Type>;


struct TypeClass;

/// <summary>
/// 型クラスへの参照
/// </summary>
using RefTypeClass = std::shared_ptr<TypeClass>;

struct TypeMap;

/// <summary>
/// 型クラスの集合としての型制約
/// </summary>
struct Constraints {
    /// <summary>
    /// 実装対象の型クラスのリスト
    /// </summary>
    std::vector<RefTypeClass> list = {};

    /// <summary>
    /// <para>型制約をマージする</para>
    /// <para>型クラスの継承関係により縮約を適用行いながらマージを行うため縮約を行わない場合は直接constraintsを操作する</para>
    /// </summary>
    /// <param name="constraints">マージ対象の型クラス</param>
    void merge(const std::vector<RefTypeClass>& constraints);

    /// <summary>
    /// 制約を持つか検査
    /// </summary>
    /// <param name="typeClass">制約を持つか検査する型クラス</param>
    [[nodiscard]] bool has(RefTypeClass typeClass) const;

    /// <summary>
    /// 指定されたクラスメソッドを抽出する
    /// </summary>
    /// <param name="name">クラスメソッド名</param>
    /// <returns>
    /// <para>指定されたクラスメソッドを実装した型クラスとconstraintsのインデックスのペア</para>
    /// <para>constraintsに存在しないが基底から型クラスを取得した場合はconstraintsのサイズがインデックスとなる</para>
    /// </returns>
    [[nodiscard]] std::pair<std::optional<RefTypeClass>, typename std::vector<RefTypeClass>::size_type> getClassMethod(std::string& name) const;
};

/// <summary>
/// 型の表現
/// </summary>
struct Type {

    /// <summary>
    /// 基底型
    /// </summary>
    struct Base {
        /// <summary>
        /// 型名
        /// </summary>
        const std::string name;
    };

    /// <summary>
    /// 関数型
    /// </summary>
    struct Function {
        /// <summary>
        /// 基底型としての意味を示す型
        /// </summary>
        RefType base;

        /// <summary>
        /// 引数型
        /// </summary>
        RefType paramType;

        /// <summary>
        /// 戻り値型
        /// </summary>
        RefType returnType;
    };

    /// <summary>
    /// 型変数
    /// </summary>
    struct Variable {
        /// <summary>
        /// 型制約
        /// </summary>
        Constraints constraints = {};

        /// <summary>
        /// 型変数の解決結果の型
        /// </summary>
        std::optional<RefType> solve = std::nullopt;

        /// <summary>
        /// スコープの深さ
        /// </summary>
        const std::size_t depth = 1;
    };

    /// <summary>
    /// ジェネリック型に出現する型変数
    /// </summary>
    struct Param {
        /// <summary>
        /// 型制約
        /// </summary>
        Constraints constraints = {};

        /// <summary>
        /// ジェネリック型の型変数のインデックス
        /// </summary>
        const std::size_t index = 0;
    };

    /// <summary>
    /// 型としての型クラス
    /// </summary>
    struct TypeClass {
        /// <summary>
        /// 型として扱う型クラス
        /// </summary>
        Constraints typeClasses = {};
    };

    // 本来はこの他にタプルやリスト、エイリアスなどの組み込み機能を定義する


    /// <summary>
    /// 型の固有情報を示す型
    /// </summary>
    using kind_type = std::variant<Base, Function, Variable, Param, TypeClass>;

    /// <summary>
    /// 型の固有情報
    /// </summary>
    kind_type kind;

    /// <summary>
    /// 型名の取得
    /// </summary>
    /// <returns>トップレベルで型が一意な場合にのみ型名を返す</returns>
    [[nodiscard]] std::optional<const std::string*> getTypeName() const {
        if (std::holds_alternative<Type::Base>(this->kind)) {
            return std::addressof(std::get<Type::Base>(this->kind).name);
        }
        else if (std::holds_alternative<Type::Function>(this->kind)) {
            return std::get<Type::Function>(this->kind).base->getTypeName();
        }
        return std::nullopt;
    }

    /// <summary>
    /// 型に紐づく型クラスのリストの取得
    /// </summary>
    /// <param name="typeMap">型表</param>
    /// <returns>型に紐づく型クラスのリスト</returns>
    [[nodiscard]] const Constraints& getTypeClassList(TypeMap& typeMap) const;
};

/// <summary>
/// ジェネリック型
/// </summary>
struct Generic {
    /// <summary>
    /// 型変数のリスト
    /// </summary>
    std::vector<RefType> vals;

    /// <summary>
    /// 型変数valsを内部で持つ型
    /// </summary>
    RefType type;
};

/// <summary>
/// 解決済みの型を取得する
/// </summary>
/// <param name="type">チェックを行う型</param>
/// <returns>解決済みの型</returns>
RefType solved(RefType type) {
    if (std::holds_alternative<Type::Variable>(type->kind)) {
        auto& val = std::get<Type::Variable>(type->kind);
        if (val.solve) {
            // 解決結果が再適用されないように適用しておく
            return val.solve.emplace(solved(val.solve.value()));
        }
    }
    return type;
}

/// <summary>
/// 型環境
/// </summary>
struct TypeEnvironment {
    /// <summary>
    /// スコープにおいて1つ上の型環境
    /// </summary>
    TypeEnvironment* parent = nullptr;

    /// <summary>
    /// スコープの深さ
    /// </summary>
    std::size_t depth = 1;

    /// <summary>
    /// 型環境についての識別子と型のペアの表
    /// </summary>
    std::unordered_map<std::string, std::variant<RefType, Generic>> map = {};

    /// <summary>
    /// 識別子の名称から型を取り出す
    /// </summary>
    /// <param name="name">識別子の名称</param>
    /// <returns>nameに対応する型</returns>
    [[nodiscard]] std::optional<const std::variant<RefType, Generic>*> lookup(const std::string& name) const {
        if (auto itr = this->map.find(name); itr != this->map.end()) {
            // std::optionalは参照型は返せないのでポインタを返す
            return std::addressof(itr->second);
        }
        else if (this->depth != 0 && this->parent) {
            // 見つからないかつ1つ上の型環境が存在するならそこから取得
            return this->parent->lookup(name);
        }
        return std::nullopt;
    }

    /// <summary>
    /// 型の生成
    /// </summary>
    /// <param name="kind">型の固有情報</param>
    /// <returns>生成した型</returns>
    [[nodiscard]] RefType newType(Type::kind_type&& kind) {
        return RefType(new Type({ .kind = std::move(kind) }));
    }

    /// <summary>
    /// 自由な型変数について型をgeneralizeする
    /// </summary>
    /// <param name="type">複製対象の型</param>
    /// <param name="env">型環境</param>
    /// <param name="vals">generalize対象の型変数のリスト</param>
    /// <returns>複製結果</returns>
    [[nodiscard]] std::variant<RefType, Generic> generalize(RefType type, std::vector<RefType> vals = std::vector<RefType>());

    /// <summary>
    /// ジェネリック型の型変数についてinstantiateする
    /// </summary>
    /// <param name="typeMap">型表</param>
    /// <param name="type">複製対象の型</param>
    /// <param name="vals">型変数へ適用対象の型</param>
    /// <returns>複製結果</returns>
    [[nodiscard]] RefType instantiate(TypeMap& typeMap, const Generic& type, std::vector<RefType> vals = std::vector<RefType>());
};

/// <summary>
/// 自由な型変数について型をgeneralizeする
/// </summary>
/// <param name="type">複製対象の型</param>
/// <param name="env">型環境</param>
/// <param name="vals">generalize対象の型変数のリスト</param>
/// <returns>複製結果</returns>
[[nodiscard]] std::variant<RefType, Generic> TypeEnvironment::generalize(RefType type, std::vector<RefType> vals) {
    std::unordered_map<RefType, typename std::vector<RefType>::size_type> map;

    struct fn {
        RefType& t;
        TypeEnvironment& e;
        std::vector<RefType>& v;
        std::unordered_map<RefType, typename std::vector<RefType>::size_type>& m;

        RefType operator()([[maybe_unused]] Type::Base& x) {
            // generalizeしない
            return this->t;
        }
        RefType operator()(Type::Function& x) {
            // 引数型と戻り値型をgeneralizeする
            auto genParamType = std::visit(fn{ .t = x.paramType, .e = this->e, .v = this->v, .m = this->m }, x.paramType->kind);
            if (x.paramType != genParamType) {
                x.paramType = genParamType;
            }
            auto genReturnType = std::visit(fn{ .t = x.returnType, .e = this->e, .v = this->v, .m = this->m }, x.returnType->kind);
            if (x.returnType != genReturnType) {
                x.returnType = genReturnType;
            }
            return this->t;
        }
        RefType operator()(Type::Variable& x) {
            if (x.solve) {
                // 解決済みの型変数の場合は解決結果に対してgeneralizeする
                // この際にGeneric型から完全に型変数を除去するために簡約
                this->t = solved(x.solve.value());
                return std::visit(fn{ .t = this->t, .e = this->e, .v = this->v, .m = this->m }, this->t->kind);
            }

            if (this->e.depth < x.depth) {
                // 自由な型変数のためgeneralizeする
                if (auto itr = this->m.find(this->t); itr != this->m.end()) {
                    return this->v[itr->second];
                }
                else {
                    this->m.insert({ this->t, this->v.size() });
                    // 導出した型変数に対する型制約は継承する
                    this->v.push_back(this->e.newType(Type::Param{ .constraints = std::move(x.constraints), .index = this->v.size() }));
                    return this->v.back();
                }
            }
            else {
                // 束縛された型変数のためgeneralizeしない
                return this->t;
            }
        }
        RefType operator()([[maybe_unused]] Type::Param& x) {
            // 外のスコープから与えられた型変数のためgeneralizeしない
            // 現状の実装ではこのシチュエーションは(おそらく)生じない
            return this->t;
        }
        RefType operator()([[maybe_unused]] Type::TypeClass& x) {
            // 型としての型クラスは型変数に依存しないためgeneralizeしない
            return this->t;
        }
    };

    auto t = std::visit(fn{ .t = type, .e = *this, .v = vals, .m = map }, type->kind);
    if (vals.size() > 0) {
        return Generic{ .vals = std::move(vals), .type = std::move(t) };
    }
    else {
        return t;
    }
}

/// <summary>
/// <para>型クラス</para>
/// <para>型クラスごとに一意のインスタンスを持つ</para>
/// </summary>
struct TypeClass {
    /// <summary>
    /// 型クラス名
    /// </summary>
    const std::string name;

    /// <summary>
    /// <para>基底</para>
    /// <para>基底のtypeはthisのtypeと一致する</para>
    /// </summary>
    Constraints bases = {};

    /// <summary>
    /// <para>型クラスを実装する対象の型を示す型変数</para>
    /// <para>制約なしの空なType::Paramである必要がある</para>
    /// </summary>
    RefType type;

    /// <summary>
    /// <para>クラスメソッド</para>
    /// <para>第一引数はtype(self)固定</para>
    /// </summary>
    std::unordered_map<std::string, std::variant<RefType, Generic>> methods = {};

    /// <summary>
    /// thisが指定された型クラスを継承しているか判定
    /// </summary>
    /// <param name="typeClass">継承しているか判定する対象の型クラス</param>
    /// <returns>継承している場合はtrue、継承していない場合はfalse</returns>
    [[nodiscard]] bool derived(RefTypeClass typeClass) const {
        // 型クラス/インスタンスの一意性に基づき評価する
        if (this == typeClass.get()) {
            return true;
        }
        for (const auto& e : this->bases.list) {
            if (e->derived(typeClass)) {
                return true;
            }
        }
        return false;
    }
};

/// <summary>
/// <para>型制約をマージする</para>
/// <para>型クラスの継承関係により縮約を適用行いながらマージを行うため縮約を行わない場合は直接constraintsを操作する</para>
/// </summary>
/// <param name="constraints">マージ対象の型クラス</param>
void Constraints::merge(const std::vector<RefTypeClass>& constraints) {
    if (constraints.empty()) {
        return;
    }

    if (this->list.empty()) {
        // もともと制約がない場合は単純コピーをする
        this->list = constraints;
    }
    else {
        auto size = this->list.size();
        // 全ての型制約を検査してより制約が強いものに置き換える
        for (auto& constraint : constraints) {
            for (decltype(size) i = 0; i < size; ++i) {
                // constraintもしくはconstraintの部分型クラスである場合はスキップする
                if (constraint != this->list[i] || this->list[i]->derived(constraint)) {
                    continue;
                }

                if (constraint->derived(this->list[i])) {
                    if (constraint != this->list[i]) {
                        this->list[i] = constraint;
                    }
                }
                else {
                    // 存在しない制約の場合はその制約を追加する
                    this->list.push_back(constraint);
                }
            }
        }
    }
}

/// <summary>
/// 制約を持つか検査
/// </summary>
/// <param name="typeClass">制約を持つか検査する型クラス</param>
[[nodiscard]] bool Constraints::has(RefTypeClass typeClass) const {
    // 全てのtypeが実装している型クラスについて検査する
    for (const auto& typeClass2 : this->list) {
        if (typeClass2->derived(typeClass)) {
            return true;
        }
    }
    return false;
}

/// <summary>
/// 指定されたクラスメソッドを抽出する
/// </summary>
/// <param name="name">クラスメソッド名</param>
/// <returns>
/// <para>指定されたクラスメソッドを実装した型クラスとconstraintsのインデックスのペア</para>
/// <para>constraintsに存在しないが基底から型クラスを取得した場合はconstraintsのサイズがインデックスとなる</para>
/// </returns>
[[nodiscard]] std::pair<std::optional<RefTypeClass>, typename std::vector<RefTypeClass>::size_type> Constraints::getClassMethod(std::string& name) const {
    for (decltype(this->list.size()) i = 0; i < this->list.size(); ++i) {
        // constraints[i]がnameをクラスメソッドとして定義しているか取得
        auto [typeClass, index] = ([&]() -> std::pair<std::optional<RefTypeClass>, typename std::vector<RefTypeClass>::size_type> {
            if (auto itr = this->list[i]->methods.find(name); itr != this->list[i]->methods.end()) {
                return { this->list[i], i };
            }
            else {
                // 基底に存在するかを探索
                return { this->list[i]->bases.getClassMethod(name).first, this->list.size() };
            }
        })();

        if (typeClass) {
            // 1つ目のnameという名称のクラスメソッドが見つかった場合は2つ目以降が存在するか確認する
            for (decltype(this->list.size()) j = i + 1; j < this->list.size(); ++j) {
                // nameというクラスメソッドを定義している型クラスが存在する場合は返却するインデックスを更新する
                if (typeClass.value() == this->list[j]) {
                    index = j;
                    continue;
                }

                // nameというクラスメソッドを定義している型クラスの基底はnameというクラスメソッドを定義していても無視する
                // 要は基底よりも派生の方をクラスメソッドの探索対象として優先する
                if (!typeClass.value()->derived(this->list[j]) && this->list[j]->methods.contains(name)) {
                    if (this->list[j]->derived(typeClass.value())) {
                        return { this->list[j], j };
                    }
                    throw std::runtime_error(std::format("クラスメソッドが一意に特定できない：{}", name));
                }
            }
            return { typeClass, index };
        }
    }
    return { std::nullopt, this->list.size() };
}

/// <summary>
/// 型に関するデータ
/// </summary>
struct TypeData {
    /// <summary>
    /// 型の表現
    /// </summary>
    std::variant<RefType, Generic> type;

    /// <summary>
    /// 実装している型クラス(制約)
    /// </summary>
    Constraints typeclasses = {};
};

/// <summary>
/// 型表
/// </summary>
struct TypeMap {
    /// <summary>
    /// 型の表
    /// </summary>
    std::unordered_map<std::string, TypeData> typeMap = {};

    /// <summary>
    /// 型クラスの表
    /// </summary>
    std::unordered_map<std::string, RefTypeClass> typeClassMap = {};

    /// <summary>
    /// <para>組込み型の定義</para>
    /// <para>型の生成時は必ずこれを経由する</para>
    /// </summary>
    struct {
        /// <summary>
        /// 関数型
        /// </summary>
        Generic fn;
    } builtin;

    /// <summary>
    /// 型定義の追加
    /// </summary>
    /// <param name="type">追加する型</param>
    /// <returns>追加した型</returns>
    auto& addType(RefType type) {
        // 型から型名を取り出して登録する
        auto typeName = type->getTypeName();
        assert(!!typeName);
        auto [itr, ret] = this->typeMap.insert({ *typeName.value(), { .type = type }});
        if (!ret) {
            throw std::runtime_error(std::format("型{}が多重定義された", *typeName.value()));
        }
        return *itr;
    }

    /// <summary>
    /// 型定義の追加
    /// </summary>
    /// <param name="type">追加する型</param>
    /// <returns>追加した型</returns>
    auto& addType(const Generic& type) {
        // 型から型名を取り出して登録する
        auto typeName = type.type->getTypeName();
        assert(!!typeName);
        auto [itr, ret] = this->typeMap.insert({ *typeName.value(), { .type = type } });
        if (!ret) {
            throw std::runtime_error(std::format("型{}が多重定義された", *typeName.value()));
        }
        return *itr;
    }

    /// <summary>
    /// 型クラスの定義の追加
    /// </summary>
    /// <param name="typeClass">追加する型クラス</param>
    /// <returns>追加した型クラス</returns>
    auto& addTypeClass(RefTypeClass typeClass) {
        auto [itr, ret] = this->typeClassMap.insert({ typeClass->name, typeClass });
        if (!ret) {
            throw std::runtime_error(std::format("型クラス{}が多重定義された", typeClass->name));
        }
        return *itr;
    }

    /// <summary>
    /// 型に制約としての型クラスを適用する
    /// </summary>
    /// <param name="type">適用対象の型</param>
    /// <param name="typeClass">適用対象の型クラス</param>
    void applyConstraint(RefType type, const std::vector<RefTypeClass>& typeClasses) {

        // 解決済みの型変数が存在すればそれを適用してから制約の適用を行う
        auto t = solved(type);

        if (std::holds_alternative<Type::Variable>(t->kind)) {
            // 通常の型変数に対しては後からでも自由に型制約を追加可能

            // 型制約にtypeClassesを追加
            std::get<Type::Variable>(t->kind).constraints.merge(typeClasses);
        }
        else {
            // トップレベルが型変数ではない通常の型の場合は後から型制約の追加は禁止
            // 型がtypeClassesを実装しているか検査する

            auto& constraints = t->getTypeClassList(*this);

            for (auto& typeClass : typeClasses) {
                // typeがtypeClassを実装しているか検査する
                // 実装していない型クラスが見つかった場合は異常とする
                if (!constraints.has(typeClass)) {
                    // typeClassMapから型クラス名の逆引きを試みる
                    if (auto itr = std::ranges::find_if(this->typeClassMap, [&typeClass](auto& p) { return p.second == typeClass; }); itr != this->typeClassMap.end()) {
                        if (std::holds_alternative<Type::Param>(t->kind)) {
                            throw std::runtime_error(std::format("ジェネリック型における型変数は事前に制約{}の宣言が必要", itr->first));
                        }
                        throw std::runtime_error(std::format("型クラス{}を実装していない", itr->first));
                    }
                    else {
                        // 匿名の型クラス（未実装）を制約として課そうとしたときの異常のため
                        // ここに到達することは論理エラーとする
                        assert(false);
                    }
                }
            }
        }
    }
};

/// <summary>
/// 型に紐づく型クラスのリストの取得
/// </summary>
/// <param name="typeMap">型表</param>
/// <returns>型に紐づく型クラスのリスト</returns>
const Constraints& Type::getTypeClassList(TypeMap& typeMap) const {
    // 型変数の場合は型制約としての型クラスのリストを返す
    if (std::holds_alternative<Type::Variable>(this->kind)) {
        return std::get<Type::Variable>(this->kind).constraints;
    }
    else if (std::holds_alternative<Type::Param>(this->kind)) {
        return std::get<Type::Param>(this->kind).constraints;
    }
    else if (std::holds_alternative<Type::TypeClass>(this->kind)) {
        // 型としての型クラスはそのまま保持している型クラスのリストを返す
        return std::get<Type::TypeClass>(this->kind).typeClasses;
    }

    // それ以外の型(明示的に型名を持つ型)は型表にアクセスして取得する
    auto name = this->getTypeName();
    assert(!!name);
    assert(typeMap.typeMap.contains(*name.value()));

    return typeMap.typeMap.at(*name.value()).typeclasses;
}

/// <summary>
/// ジェネリック型の型変数についてinstantiateする
/// </summary>
/// <param name="typeMap">型表</param>
/// <param name="type">複製対象の型</param>
/// <param name="vals">型変数へ適用対象の型</param>
/// <returns>複製結果</returns>
[[nodiscard]] RefType TypeEnvironment::instantiate(TypeMap& typeMap, const Generic& type, std::vector<RefType> vals) {
    // instantiateした対象の型変数のリスト
    vals.resize(type.vals.size());

    // デフォルトでvalsが要素をもつときは型制約を適用・検査する
    for (decltype(type.vals.size()) i = 0; i < type.vals.size(); ++i) {
        auto& param = std::get<Type::Param>(type.vals[i]->kind);
        if (!vals[i]) {
            // 型が割当たっていない場合は型変数を割り当てる
            // 型変数に課された制約を継承する
            vals[i] = this->newType(Type::Variable{ .constraints = param.constraints, .depth = this->depth });
        }
        else {
            typeMap.applyConstraint(vals[i], param.constraints.list);
        }
    }

    struct fn {
        RefType t;
        TypeEnvironment& e;
        const std::vector<RefType>& v;
        const std::vector<RefType>& p;

        RefType operator()([[maybe_unused]] const Type::Base& x) {
            // instantiateしない
            return this->t;
        }
        RefType operator()(const Type::Function& x) {
            // 引数型と戻り値型をinstantiateする
            auto instParamType = std::visit(fn{ .t = x.paramType, .e = this->e, .v = this->v, .p = this->p }, x.paramType->kind);
            auto instReturnType = std::visit(fn{ .t = x.returnType, .e = this->e, .v = this->v, .p = this->p }, x.returnType->kind);

            if (x.paramType == instParamType && x.returnType == instReturnType) {
                // instantiateされなかった場合は新規にインスタンスを生成しない
                return this->t;
            }
            else {
                // instantiateが生じた場合は新規にインスタンスを生成する
                // 組込み型である関数型なので基底を継承する
                return this->e.newType(Type::Function{ .base = x.base, .paramType = std::move(instParamType), .returnType = std::move(instReturnType) });
            }
        }
        RefType operator()([[maybe_unused]] const Type::Variable& x) {
            // 外のスコープから与えられた型変数のためinstantiateしない
            return this->t;
        }
        RefType operator()(const Type::Param& x) {
            // ジェネリック型の型変数と等しい場合はinstantiateする
            if (x.index < this->p.size() && this->p[x.index] == this->t) {
                return this->v[x.index];
            }
            else {
                return this->t;
            }
        }
        RefType operator()([[maybe_unused]] const Type::TypeClass& x) {
            // 型としての型クラスは型変数に依存しないためinstantiateしない
            return this->t;
        }
    };

    return std::visit(fn{ .t = type.type, .e = *this, .v = vals, .p = type.vals }, type.type->kind);
}

/// <summary>
/// typeがtargetに依存しているかの判定(参照先の型が一致するかを判定する)
/// </summary>
/// <param name="type">検査対象の型1</param>
/// <param name="target">検査対象の型2</param>
/// <returns>typeがtargetに依存している場合にtrue、依存していない場合にfalse</returns>
[[nodiscard]] bool depend(const RefType& type, const RefType& target) {
    struct fn {
        const RefType& t;

        bool operator()([[maybe_unused]] const Type::Base& x) {
            return false;
        }
        bool operator()(const Type::Function& x) {
            return x.paramType == this->t || x.returnType == this->t || depend(x.paramType, this->t) || depend(x.returnType, this->t);
        }
        bool operator()([[maybe_unused]] const Type::Variable& x) {
            return x.solve ? depend(x.solve.value(), this->t) : false;
        }
        bool operator()([[maybe_unused]] const Type::Param& x) {
            return false;
        }
        bool operator()([[maybe_unused]] const Type::TypeClass& x) {
            return false;
        }
    };

    return type == target || std::visit(fn{ .t = target }, type->kind);
}

/// <summary>
/// <para>副作用付きの2つの型の単一化</para>
/// <para>暗黙の型変換はtype1 <- type2の方向で行われる</para>
/// </summary>
/// <param name="typeMap">型表</param>
/// <param name="type1">単一化の対象の型1</param>
/// <param name="type2">単一化の対象の型2</param>
void unify(TypeMap& typeMap, RefType type1, RefType type2) {

    // 解決済みの型変数が存在すればそれを適用してから単一化を行う
    auto t1 = solved(type1);
    auto t2 = solved(type2);

    if (t1 != t2) {
        if (std::holds_alternative<Type::Variable>(t1->kind)) {
            auto& t1v = std::get<Type::Variable>(t1->kind);

            if (std::holds_alternative<Type::Variable>(t2->kind)) {
                // 型変数同士の場合は型の循環が起きないように外のスコープのものを設定
                auto& t2v = std::get<Type::Variable>(t2->kind);
                if (t1v.depth < t2v.depth) {
                    // 型制約をマージする
                    t1v.constraints.merge(t2v.constraints.list);
                    t2v.solve = t1;
                }
                else {
                    // 型制約をマージする
                    t2v.constraints.merge(t1v.constraints.list);
                    t1v.solve = t2;
                }
            }
            else {
                if (depend(t1, t2)) {
                    // 再帰的な単一化は決定不能のため異常(ex. x -> x xのような関数)
                    throw std::runtime_error("再帰的単一化");
                }
                // 一方のみが型変数の場合はもう一方と型を一致させる
                // t2がt1の制約を満たすかを検証する
                typeMap.applyConstraint(t2, t1v.constraints.list);
                t1v.solve = t2;
            }
        }
        else {
            if (std::holds_alternative<Type::Variable>(t2->kind)) {
                auto& t2v = std::get<Type::Variable>(t2->kind);
                if (depend(t2, t1)) {
                    // 再帰的な単一化は決定不能のため異常(ex. x -> x xのような関数)
                    throw std::runtime_error("再帰的単一化");
                }
                // 一方のみが型変数の場合はもう一方と型を一致させる
                // t1がt2の制約を満たすかを検証する
                typeMap.applyConstraint(t1, t2v.constraints.list);
                t2v.solve = t1;
            }
            else if (std::holds_alternative<Type::TypeClass>(t1->kind)) {
                // type1 <- type2な暗黙の型変換の検証
                // type2はType::Variableでないためtype1に課された型クラスがtype2で実装されているか検証となる
                typeMap.applyConstraint(t2, std::get<Type::TypeClass>(t1->kind).typeClasses.list);

                // キャストが生じた場合の対処は型推論の域を逸脱するため未実装
                // 実装する場合は明示的なキャストの構文を構文木に挿入する
            }
            else {
                // 型が一致する場合に部分型について再帰的に単一化
                // 制約付きの場合は「C ⊨ type1 ∼ type2」のような同値の条件によりキャストを行うが未実装
                if (t1->kind.index() == t2->kind.index()) {
                    if (std::holds_alternative<Type::Function>(t1->kind)) {
                        auto& k1 = std::get<Type::Function>(t1->kind);
                        auto& k2 = std::get<Type::Function>(t2->kind);
                        unify(typeMap, k1.paramType, k2.paramType);
                        unify(typeMap, k1.returnType, k2.returnType);
                    }
                    else {
                        // 部分型をもたないため型は一致しない
                        throw std::runtime_error("型の不一致");
                    }
                }
                else {
                    // 型の種類が一致しない
                    throw std::runtime_error("型の不一致");
                }
            }
        }
    }
}

/// <summary>
/// 式を示す構文木
/// </summary>
struct Expression {
    virtual ~Expression() {};

    /// <summary>
    /// Algorithm Jの適用
    /// </summary>
    /// <param name="typeMap">型表</param>
    /// <param name="env">型環境</param>
    /// <returns>評価結果の型</returns>
    virtual RefType J(TypeMap& typeMap, TypeEnvironment& env) = 0;

    /// <summary>
    /// Algorithm Mの適用
    /// </summary>
    /// <param name="typeMap">型表</param>
    /// <param name="env">型環境</param>
    /// <param name="rho">式が推測される型</param>
    virtual void M(TypeMap& typeMap, TypeEnvironment& env, RefType rho) = 0;
};

/// <summary>
/// 定数を示す構文木(簡単のために値はもたない)
/// </summary>
struct Constant : Expression {
    /// <summary>
    /// 定数の型
    /// </summary>
    RefType b;

    Constant(RefType b) : b(b) {}
    ~Constant() override {}

    /// <summary>
    /// Algorithm Jの適用
    /// </summary>
    /// <param name="typeMap">型表</param>
    /// <param name="env">型環境</param>
    /// <returns>評価結果の型</returns>
    RefType J([[maybe_unused]] TypeMap& typeMap, [[maybe_unused]] TypeEnvironment& env) override {
        return this->b;
    }

    /// <summary>
    /// Algorithm Mの適用
    /// </summary>
    /// <param name="typeMap">型表</param>
    /// <param name="env">型環境</param>
    /// <param name="rho">式が推測される型</param>
    void M(TypeMap& typeMap, [[maybe_unused]] TypeEnvironment& env, RefType rho) override {
        unify(typeMap, rho, this->b);
    }
};

/// <summary>
/// 識別子を示す構文木
/// </summary>
struct Identifier : Expression {
    /// <summary>
    /// 識別子名
    /// </summary>
    std::string x;

    Identifier(std::string_view x) : x(x) {}
    ~Identifier() override {}

    /// <summary>
    /// Algorithm Jの適用
    /// </summary>
    /// <param name="typeMap">型表</param>
    /// <param name="env">型環境</param>
    /// <returns>評価結果の型</returns>
    RefType J(TypeMap& typeMap, TypeEnvironment& env) override {
        struct fn {
            TypeMap& m;
            TypeEnvironment& e;

            RefType operator()(const RefType& x) {
                return x;
            }
            RefType operator()(const Generic& x) {
                // 多相のためにinstantiateする(単相の場合は不要)
                return this->e.instantiate(this->m, x);
            }
        };

        // 型環境から型を取り出す
        auto tau = env.lookup(this->x);
        if (tau) {
            return std::visit(fn{ .m = typeMap, .e = env }, *tau.value());
        }
        throw std::runtime_error(std::format("不明な識別子：{}", this->x));
    }

    /// <summary>
    /// Algorithm Mの適用
    /// </summary>
    /// <param name="typeMap">型表</param>
    /// <param name="env">型環境</param>
    /// <param name="rho">式が推測される型</param>
    void M(TypeMap& typeMap, TypeEnvironment& env, RefType rho) override {
        struct fn {
            TypeMap& m;
            TypeEnvironment& e;

            RefType operator()(const RefType& x) {
                return x;
            }
            RefType operator()(const Generic& x) {
                // 多相のためにinstantiateする(単相の場合は不要)
                return this->e.instantiate(this->m, x);
            }
        };

        // 型環境から型を取り出す
        auto tau = env.lookup(this->x);

        if (tau) {
            unify(typeMap, rho, std::visit(fn{ .m = typeMap, .e = env }, *tau.value()));
        }
        else {
            throw std::runtime_error(std::format("不明な識別子：{}", this->x));
        }
    }
};

/// <summary>
/// ラムダ抽象を示す構文木
/// </summary>
struct Lambda : Expression {
    /// <summary>
    /// 引数名
    /// </summary>
    std::string x;
    /// <summary>
    /// xの型制約
    /// </summary>
    std::optional<RefType> constraint = std::nullopt;
    /// <summary>
    /// 関数本体の式
    /// </summary>
    std::shared_ptr<Expression> e;

    Lambda(std::string_view x, std::shared_ptr<Expression> e) : x(x), e(e) {}
    Lambda(std::string_view x, RefType constraint, std::shared_ptr<Expression> e) : x(x), constraint(constraint), e(e) {}
    ~Lambda() override {}

    /// <summary>
    /// Algorithm Jの適用
    /// </summary>
    /// <param name="typeMap">型表</param>
    /// <param name="env">型環境</param>
    /// <returns>評価結果の型</returns>
    RefType J(TypeMap& typeMap, TypeEnvironment& env) override {
        // 型環境を新しく構成
        // 一引数単位で型環境を構築するのは効率が悪いため通常は複数の引数を一度に扱う
        TypeEnvironment newEnv = {
            .parent = std::addressof(env),
            .depth = env.depth + 1
        };

        // 型環境にxを登録してeを評価
        auto t = this->constraint ? this->constraint.value() : newEnv.newType(Type::Variable{ .depth = newEnv.depth });
        newEnv.map.insert({ this->x, t });
        auto tau = this->e->J(typeMap, newEnv);

        return env.instantiate(typeMap, typeMap.builtin.fn, { t, tau });
    }

    /// <summary>
    /// Algorithm Mの適用
    /// </summary>
    /// <param name="typeMap">型表</param>
    /// <param name="env">型環境</param>
    /// <param name="rho">式が推測される型</param>
    void M(TypeMap& typeMap, TypeEnvironment& env, RefType rho) override {
        // 型環境を新しく構成
        // 一引数単位で型環境を構築するのは効率が悪いため通常は複数の引数を一度に扱う
        TypeEnvironment newEnv = {
            .parent = std::addressof(env),
            .depth = env.depth + 1
        };

        auto t1 = this->constraint ? this->constraint.value() : newEnv.newType(Type::Variable{ .depth = newEnv.depth });
        auto t2 = newEnv.newType(Type::Variable{ .depth = newEnv.depth });
        unify(typeMap, rho, env.instantiate(typeMap, typeMap.builtin.fn, { t1, t2 }));

        // 型環境にxを登録してeを評価
        newEnv.map.insert({ this->x, t1 });
        this->e->M(typeMap, newEnv, t2);
    }
};

/// <summary>
/// 関数適用を示す構文木
/// </summary>
struct Apply : Expression {
    /// <summary>
    /// 関数を示す式
    /// </summary>
    std::shared_ptr<Expression> e1;
    /// <summary>
    /// 引数の式
    /// </summary>
    std::shared_ptr<Expression> e2;

    Apply(std::shared_ptr<Expression> e1, std::shared_ptr<Expression> e2) : e1(e1), e2(e2) {}
    ~Apply() override {}

    /// <summary>
    /// Algorithm Jの適用
    /// </summary>
    /// <param name="typeMap">型表</param>
    /// <param name="env">型環境</param>
    /// <returns>評価結果の型</returns>
    RefType J(TypeMap& typeMap, TypeEnvironment& env) override {
        auto tau1 = this->e1->J(typeMap, env);
        auto tau2 = this->e2->J(typeMap, env);
        auto t = env.newType(Type::Variable{ .depth = env.depth });

        unify(typeMap, tau1, env.instantiate(typeMap, typeMap.builtin.fn, { tau2, t }));

        return t;
    }

    /// <summary>
    /// Algorithm Mの適用
    /// </summary>
    /// <param name="typeMap">型表</param>
    /// <param name="env">型環境</param>
    /// <param name="rho">式が推測される型</param>
    void M(TypeMap& typeMap, TypeEnvironment& env, RefType rho) override {
        auto t = env.newType(Type::Variable{ .depth = env.depth });

        this->e1->M(typeMap, env, env.instantiate(typeMap, typeMap.builtin.fn, { t, rho }));
        this->e2->M(typeMap, env, t);
    }
};

/// <summary>
/// Let束縛を示す構文木
/// </summary>
struct Let : Expression {
    /// <summary>
    /// 束縛先の識別子名
    /// </summary>
    std::string x;
    /// <summary>
    /// 明示的に宣言されたジェネリック型に出現する型変数
    /// </summary>
    std::vector<RefType> params = {};
    /// <summary>
    /// 束縛する式
    /// </summary>
    std::shared_ptr<Expression> e1;
    /// <summary>
    /// xを利用する式
    /// </summary>
    std::shared_ptr<Expression> e2;

    Let(std::string_view x, std::shared_ptr<Expression> e1, std::shared_ptr<Expression> e2) : x(x), e1(e1), e2(e2) {}
    Let(std::string_view x, const std::vector<RefType>& params, std::shared_ptr<Expression> e1, std::shared_ptr<Expression> e2) : x(x), params(params), e1(e1), e2(e2) {}
    ~Let() override {}

    /// <summary>
    /// Algorithm Jの適用
    /// </summary>
    /// <param name="typeMap">型表</param>
    /// <param name="env">型環境</param>
    /// <returns>評価結果の型</returns>
    RefType J(TypeMap& typeMap, TypeEnvironment& env) override {
        auto tau1 = this->e1->J(typeMap, env);
        // xが定義済みであっても型環境の改装を無視して上書きする
        // グローバルな型環境の場合は異常にする等があるかもしれない
        env.map.insert_or_assign(this->x, env.generalize(tau1, this->params));

        return this->e2->J(typeMap, env);
    }

    /// <summary>
    /// Algorithm Mの適用
    /// </summary>
    /// <param name="typeMap">型表</param>
    /// <param name="env">型環境</param>
    /// <param name="rho">式が推測される型</param>
    void M(TypeMap& typeMap, TypeEnvironment& env, RefType rho) override {
        auto t = env.newType(Type::Variable{ .depth = env.depth });

        this->e1->M(typeMap, env, t);

        // xが定義済みであっても型環境の改装を無視して上書きする
        // グローバルな型環境の場合は異常にする等があるかもしれない
        env.map.insert_or_assign(this->x, env.generalize(t, this->params));
        this->e2->M(typeMap, env, rho);
    }
};

/// <summary>
/// Letrec束縛を示す構文木
/// </summary>
struct Letrec : Expression {
    /// <summary>
    /// 束縛先の識別子名
    /// </summary>
    std::string x;
    /// <summary>
    /// 明示的に宣言されたジェネリック型に出現する型変数
    /// </summary>
    std::vector<RefType> params = {};
    /// <summary>
    /// 束縛する式
    /// </summary>
    std::shared_ptr<Expression> e1;
    /// <summary>
    /// xを利用する式
    /// </summary>
    std::shared_ptr<Expression> e2;

    Letrec(std::string_view x, std::shared_ptr<Expression> e1, std::shared_ptr<Expression> e2) : x(x), e1(e1), e2(e2) {}
    Letrec(std::string_view x, const std::vector<RefType>& params, std::shared_ptr<Expression> e1, std::shared_ptr<Expression> e2) : x(x), params(params), e1(e1), e2(e2) {}
    ~Letrec() override {}

    /// <summary>
    /// Algorithm Jの適用
    /// </summary>
    /// <param name="typeMap">型表</param>
    /// <param name="env">型環境</param>
    /// <returns>評価結果の型</returns>
    RefType J(TypeMap& typeMap, TypeEnvironment& env) override {
        auto t = env.newType(Type::Variable{ .depth = env.depth });
        // xが定義済みであっても型環境の改装を無視して上書きする
        // グローバルな型環境の場合は異常にする等があるかもしれない
        env.map.insert_or_assign(this->x, t);

        auto tau1 = this->e1->J(typeMap, env);
        unify(typeMap, tau1, t);

        env.map.insert_or_assign(this->x, env.generalize(tau1, this->params));

        return this->e2->J(typeMap, env);
    }

    /// <summary>
    /// Algorithm Mの適用
    /// </summary>
    /// <param name="typeMap">型表</param>
    /// <param name="env">型環境</param>
    /// <param name="rho">式が推測される型</param>
    void M(TypeMap& typeMap, TypeEnvironment& env, RefType rho) override {
        auto t1 = env.newType(Type::Variable{ .depth = env.depth });
        auto t2 = env.newType(Type::Variable{ .depth = env.depth });
        // xが定義済みであっても型環境の改装を無視して上書きする
        // グローバルな型環境の場合は異常にする等があるかもしれない
        env.map.insert_or_assign(this->x, t1);

        this->e1->M(typeMap, env, t2);
        unify(typeMap, t1, t2);

        env.map.insert_or_assign(this->x, env.generalize(t1, this->params));
        this->e2->M(typeMap, env, rho);
    }
};

/// <summary>
/// クラスメソッドへのアクセスを示す構文木
/// </summary>
struct AccessToClassMethod : Expression {
    /// <summary>
    /// 式
    /// </summary>
    std::shared_ptr<Expression> e;

    /// <summary>
    /// クラスメソッド名
    /// </summary>
    std::string x;

    AccessToClassMethod(std::shared_ptr<Expression> e, std::string_view x) : e(e), x(x) {}
    ~AccessToClassMethod() override {}

    /// <summary>
    /// 指定された型からクラスメソッドを抽出する
    /// </summary>
    /// <param name="typeMap">型表</param>
    /// <param name="env">型環境</param>
    /// <param name="type">クラスメソッドを実装した型</param>
    /// <returns>クラスメソッドを示す型</returns>
    RefType getClassMethod(TypeMap& typeMap, TypeEnvironment& env, RefType type) {
        // typeがxをクラスメソッドとしてただ1つもつか検査
        // 型クラスを実装しているかだけを見るため、クラスメソッドの実装方式などは見ない
        auto& typeClassList = solved(type)->getTypeClassList(typeMap);
        auto [typeClass, index] = typeClassList.getClassMethod(this->x);

        if (typeClass) {
            // クラスメソッドがジェネリック型ならinstantiateする
            // 型クラスを実装する対象の型についてもinstantiateする
            auto& method = typeClass.value()->methods.at(this->x);
            auto f = env.instantiate(typeMap, Generic{
                .vals = { typeClass.value()->type },
                .type = (
                    std::holds_alternative<Generic>(method) ?
                    env.instantiate(typeMap, std::get<Generic>(method)) :
                    std::get<RefType>(method)
                )
                });
            return std::get<Type::Function>(f->kind).returnType;
        }
        else {
            throw std::runtime_error(std::format("クラスメソッドが実装されていない：{}", this->x));
        }
    }

    /// <summary>
    /// Algorithm Jの適用
    /// </summary>
    /// <param name="typeMap">型表</param>
    /// <param name="env">型環境</param>
    /// <returns>評価結果の型</returns>
    RefType J(TypeMap& typeMap, TypeEnvironment& env) override {
        auto tau = this->e->J(typeMap, env);

        // クラスメソッドを取得して部分適用結果の型を取得する
        return this->getClassMethod(typeMap, env, tau);
    }

    /// <summary>
    /// Algorithm Mの適用
    /// </summary>
    /// <param name="typeMap">型表</param>
    /// <param name="env">型環境</param>
    /// <param name="rho">式が推測される型</param>
    void M(TypeMap& typeMap, TypeEnvironment& env, RefType rho) override {
        auto t = env.newType(Type::Variable{ .depth = env.depth });
        this->e->M(typeMap, env, t);

        // クラスメソッドを取得して部分適用結果の型を取得する
        unify(typeMap, this->getClassMethod(typeMap, env, t), rho);
    }
};

/// <summary>
/// 二項演算を示す構文木
/// </summary>
struct BinaryExpression : Expression {
    /// <summary>
    /// 左項
    /// </summary>
    std::shared_ptr<Expression> lhs;
    /// <summary>
    /// 右項
    /// </summary>
    std::shared_ptr<Expression> rhs;

    BinaryExpression(std::shared_ptr<Expression> lhs, std::shared_ptr<Expression> rhs) : lhs(lhs), rhs(rhs) {}
    ~BinaryExpression() override {}

    /// <summary>
    /// クラスメソッドを抽出する
    /// </summary>
    /// <param name="typeMap">型表</param>
    /// <param name="env">型環境</param>
    /// <returns>クラスメソッドを示す型</returns>
    RefType getClassMethod(TypeMap& typeMap, TypeEnvironment& env) {
        auto& typeClass = this->getTypeClass();
        auto& methodName = this->getMethodName();
        // クラスメソッドがないのは論理エラーとする
        assert(typeClass->methods.contains(methodName));

        // クラスメソッドがジェネリック型ならinstantiateする
        // 型クラスを実装する対象の型についてもinstantiateする
        auto& method = typeClass->methods.at(methodName);
        auto f = env.instantiate(typeMap, Generic{
            .vals = { typeClass->type },
            .type = (
                std::holds_alternative<Generic>(method) ?
                env.instantiate(typeMap, std::get<Generic>(method)) :
                std::get<RefType>(method)
            )
            });
        return std::get<Type::Function>(f->kind).returnType;
    }

    /// <summary>
    /// 二項演算を示す型クラスを取得する
    /// </summary>
    /// <returns>二項演算を示す型クラス</returns>
    virtual const RefTypeClass& getTypeClass() const = 0;

    /// <summary>
    /// 二項演算をクラスメソッド名を取得する
    /// </summary>
    /// <returns>二項演算を示す型クラス</returns>
    virtual const std::string& getMethodName() const = 0;

    /// <summary>
    /// Algorithm Jの適用
    /// </summary>
    /// <param name="typeMap">型表</param>
    /// <param name="env">型環境</param>
    /// <returns>評価結果の型</returns>
    RefType J(TypeMap& typeMap, TypeEnvironment& env) override {
        // 左項については型制約の適用・検査
        auto tau1 = this->lhs->J(typeMap, env);
        typeMap.applyConstraint(tau1, { this->getTypeClass() });
        auto tau2 = this->rhs->J(typeMap, env);

        // クラスメソッドに対して単一化を行って二項演算の結果型を得る
        auto t = env.newType(Type::Variable{ .depth = env.depth });
        unify(typeMap, this->getClassMethod(typeMap, env), env.instantiate(typeMap, typeMap.builtin.fn, { tau2, t }));

        return t;
    }

    /// <summary>
    /// Algorithm Mの適用
    /// </summary>
    /// <param name="typeMap">型表</param>
    /// <param name="env">型環境</param>
    /// <param name="rho">式が推測される型</param>
    void M(TypeMap& typeMap, TypeEnvironment& env, RefType rho) override {
        auto t1 = env.newType(Type::Variable{ .depth = env.depth });
        // 左項については型制約の適用・検査
        this->lhs->M(typeMap, env, t1);
        typeMap.applyConstraint(t1, { this->getTypeClass() });

        // クラスメソッドに対して単一化を行って二項演算の結果型を得る
        auto t2 = env.newType(Type::Variable{ .depth = env.depth });
        unify(typeMap, this->getClassMethod(typeMap, env), env.instantiate(typeMap, typeMap.builtin.fn, { t2, rho }));

        this->rhs->M(typeMap, env, t2);
    }
};

/// <summary>
/// 加算を示す構文木
/// </summary>
struct Add : BinaryExpression {
    /// <summary>
    /// 加算の定義についての型クラス
    /// </summary>
    static RefTypeClass typeClass;
    /// <summary>
    /// 加算尾定義についてのクラスメソッド名
    /// </summary>
    static std::string methodName;

    using BinaryExpression::BinaryExpression;
    ~Add() override {}

    /// <summary>
    /// 二項演算を示す型クラスを取得する
    /// </summary>
    /// <returns>二項演算を示す型クラス</returns>
    const RefTypeClass& getTypeClass() const override {
        return Add::typeClass;
    }

    /// <summary>
    /// 二項演算をクラスメソッド名を取得する
    /// </summary>
    /// <returns>二項演算を示す型クラス</returns>
    const std::string& getMethodName() const override {
        return Add::methodName;
    }
};
RefTypeClass Add::typeClass = nullptr;
std::string Add::methodName = "";

/// <summary>
/// RefTypeの標準出力
/// </summary>
/// <param name="os">出力ストリーム</param>
/// <param name="type">出力対象の型</param>
/// <returns></returns>
std::ostream& operator<<(std::ostream& os, RefType type) {
    struct fn {
        std::ostream& o;
        char varCnt = 'a';
        std::unordered_map<const Type::Variable*, char> varmap = {};

        void operator()(const Type::Base& x) {
            this->o << x.name;
        }
        void operator()(const Type::Function& x) {
            // 括弧付きで出力するかを判定
            bool isSimple = std::holds_alternative<Type::Base>(x.paramType->kind) ||
                std::holds_alternative<Type::Variable>(x.paramType->kind) ||
                std::holds_alternative<Type::Param>(x.paramType->kind) ||
                std::holds_alternative<Type::TypeClass>(x.paramType->kind);
            if (isSimple) {
                std::visit(*this, x.paramType->kind);
            }
            else {
                this->o << "(";
                std::visit(*this, x.paramType->kind);
                this->o << ")";
            }
            this->o << " -> ";
            std::visit(*this, x.returnType->kind);
        }
        void operator()(const Type::Variable& x) {
            if (x.solve) {
                // 解決済みの型変数の場合は解決結果の型に対して出力
                std::visit(*this, x.solve.value()->kind);
            }
            else {
                // 雑に[a, z]の範囲で型変数を出力
                // zを超えたら全て「_」で出力
                if (auto itr = this->varmap.find(std::addressof(x)); itr != this->varmap.end()) {
                    this->o << '?' << itr->second;
                }
                else {
                    this->varmap.insert({ std::addressof(x), this->varCnt });
                    this->o << '?' << this->varCnt;
                    switch (this->varCnt) {
                    case 'z':
                        this->varCnt = '_';
                        break;
                    case '_':
                        break;
                    default:
                        ++this->varCnt;
                        break;
                    }
                }

                // 型クラスの情報を出力
                if (x.constraints.list.size() == 1) {
                    this->o << ": " << x.constraints.list[0]->name;
                }
                else if (x.constraints.list.size() > 1) {
                    this->o << ":(" << x.constraints.list[0]->name;
                    for (auto itr = x.constraints.list.begin() + 1; itr != x.constraints.list.end(); ++itr) {
                        this->o << " + " << (*itr)->name;
                    }
                    this->o << ")";
                }
            }
        }
        void operator()(const Type::Param& x) {
            // 雑に[a, z]の範囲で型変数を出力
            // zを超えたら全て「_」で出力
            std::size_t c = 'a' + x.index;
            this->o << '\'' << static_cast<char>(c <= 'z' ? c : '_');

            // 型クラスの情報を出力
            if (x.constraints.list.size() == 1) {
                this->o << ": " << x.constraints.list[0]->name;
            }
            else if (x.constraints.list.size() > 1) {
                this->o << ":(" << x.constraints.list[0]->name;
                for (auto itr = x.constraints.list.begin() + 1; itr != x.constraints.list.end(); ++itr) {
                    this->o << " + " << (*itr)->name;
                }
                this->o << ")";
            }
        }
        void operator()(const Type::TypeClass& x) {
            auto size = x.typeClasses.list.size();
            if (size == 0) {
                // 空を出力
                this->o << "()";
            }
            else {
                // 2つ以上の複数の型クラスから成る型の場合は括弧付きで出力
                if (size > 1) {
                    this->o << '(';
                }
                // 型クラスとわかるようにプレフィックスとして「:」を付けて出力する
                for (decltype(x.typeClasses.list.size()) i = 0; i < x.typeClasses.list.size(); ++i) {
                    auto& l = x.typeClasses.list[i];
                    if (auto itr = std::ranges::find_if(x.typeClasses.list, [&l](auto& typeClass) { return typeClass == l; }); itr != x.typeClasses.list.end()) {
                        if (i > 0) {
                            this->o << " + ";
                        }
                        this->o << ':' << (*itr)->name;
                    }
                    else {
                        assert(false);
                    }
                }
                if (size > 1) {
                    this->o << ')';
                }
            }
        }
    };
    std::visit(fn{ .o = os }, type->kind);

    return os;
}

// 雑に型を短く書くための関数
RefType base(TypeEnvironment& env, const std::string& name) { return env.newType(Type::Base{ .name = name }); }
RefType var(TypeEnvironment& env) { return env.newType(Type::Variable{ .depth = env.depth + 1 }); }
RefType param(TypeEnvironment& env, std::size_t index = 0) { return env.newType(Type::Param{ .index = index }); }
RefType fun(TypeEnvironment& env, RefType base, RefType paramType, RefType returnType) { return env.newType(Type::Function{ .base = base, .paramType = paramType, .returnType = returnType }); }
RefType fun(TypeMap& typeMap, TypeEnvironment& env, const Generic& base, RefType paramType, RefType returnType) { return env.instantiate(typeMap, base, { paramType, returnType }); }
template <class... Types>
RefType tc(TypeEnvironment& env, Types&&... args) { return env.newType(Type::TypeClass{ .typeClasses{.list = { std::forward<Types>(args)...}} }); }

// 雑に構文を短く書くための関数
std::shared_ptr<Expression> c(RefType type) { return std::shared_ptr<Expression>(new Constant(type)); }
std::shared_ptr<Expression> id(const std::string& name) { return std::shared_ptr<Expression>(new Identifier(name)); }
std::shared_ptr<Expression> lambda(const std::string& name, std::shared_ptr<Expression> expr) { return std::shared_ptr<Expression>(new Lambda(name, expr)); }
std::shared_ptr<Expression> lambda(const std::string& name, RefType constraint, std::shared_ptr<Expression> expr) { return std::shared_ptr<Expression>(new Lambda(name, constraint, expr)); }
std::shared_ptr<Expression> apply(std::shared_ptr<Expression> expr1, std::shared_ptr<Expression> expr2) { return std::shared_ptr<Expression>(new Apply(expr1, expr2)); }
template <class Head, class... Tail>
std::shared_ptr<Expression> apply(Head&& head1, Head&& head2, Tail&&... tail) { return apply(std::shared_ptr<Expression>(new Apply(head1, head2)), std::forward<Tail>(tail)...); }
std::shared_ptr<Expression> let(const std::string& name, std::shared_ptr<Expression> expr1, std::shared_ptr<Expression> expr2) { return std::shared_ptr<Expression>(new Let(name, expr1, expr2)); }
std::shared_ptr<Expression> let(const std::string& name, const std::vector<RefType>& params, std::shared_ptr<Expression> expr1, std::shared_ptr<Expression> expr2) { return std::shared_ptr<Expression>(new Let(name, params, expr1, expr2)); }
std::shared_ptr<Expression> letrec(const std::string& name, std::shared_ptr<Expression> expr1, std::shared_ptr<Expression> expr2) { return std::shared_ptr<Expression>(new Letrec(name, expr1, expr2)); }
std::shared_ptr<Expression> letrec(const std::string& name, const std::vector<RefType>& params, std::shared_ptr<Expression> expr1, std::shared_ptr<Expression> expr2) { return std::shared_ptr<Expression>(new Letrec(name, params, expr1, expr2)); }
std::shared_ptr<Expression> dot(std::shared_ptr<Expression> expr, const std::string& name) { return std::shared_ptr<Expression>(new AccessToClassMethod(expr, name)); }
std::shared_ptr<Expression> add(std::shared_ptr<Expression> expr1, std::shared_ptr<Expression> expr2) { return std::shared_ptr<Expression>(new Add(expr1, expr2)); }

int main() {
    // 型環境
    auto env = TypeEnvironment();
    // 型表
    auto typeMap = TypeMap();

    // 適当に数値型とBoolean型、関数を定義しておく
    typeMap.builtin.fn = std::get<Generic>(env.generalize(fun(env, base(env, "fn"), var(env), var(env))));
    typeMap.addType(typeMap.builtin.fn);
    auto& [numberN, numberTD] = typeMap.addType(base(env, "number"));
    auto& numberT = std::get<RefType>(numberTD.type);
    auto& [booleanN, booleanTD] = typeMap.addType(base(env, "boolean"));
    auto& booleanT = std::get<RefType>(booleanTD.type);

    // 組込みの型クラスを定義する
    Add::methodName = "add";
    Add::typeClass = ([&] {
        auto valT = param(env);
        return RefTypeClass(new TypeClass({
            .name = "Add",
            .type = valT,
            .methods = {
                // add :: 'a -> 'a -> 'a
                { Add::methodName, fun(typeMap, env, typeMap.builtin.fn, valT, fun(typeMap, env, typeMap.builtin.fn, valT, valT)) }
            }
        }));
    })();
    typeMap.addTypeClass(Add::typeClass);

    // 適当に型クラスを定義する
    typeMap.addTypeClass(([&typeMap, &env] {
        auto valT = param(env);
        return RefTypeClass(new TypeClass({
            .name = "TypeClass",
            .type = valT,
            .methods = {
                // method :: 'a -> 'a -> 'a
                { "method", fun(typeMap, env, typeMap.builtin.fn, valT, fun(typeMap, env, typeMap.builtin.fn, valT, valT)) }
            }
        }));
    })());
    // Boolean型に型クラスTypeClassを実装する
    booleanTD.typeclasses.list.push_back(typeMap.typeClassMap["TypeClass"]);

    // 定数のつもりの構文を宣言しておく
    auto _true = c(booleanT);

    for (auto& expr :
        {
            // n -> n + n
            // 型推論で自動的に型クラスが付加される例
            lambda("n", add(id("n"), id("n"))),
            // true.method true
            // 型クラスを実装した型からクラスメソッドを呼び出す
            apply(dot(_true, "method"), _true),
            // let f = n: (:TypeClass) -> n.method n in f
            // 引数型に型を明示的に指定してクラスメソッドを呼び出す例
            let("f", lambda("n", tc(env, typeMap.typeClassMap["TypeClass"]), apply(dot(id("n"), "method"), id("n"))), id("f")),
            // let f<'a: TypeClass> = n: 'a -> n.method n in f
            // 引数型に型変数を明示的に指定してクラスメソッドを呼び出す例
            ([&] {
                auto p0 = param(env, 0);
                std::get<Type::Param>(p0->kind).constraints.list = { typeMap.typeClassMap["TypeClass"] };
                return let("f", { p0 }, lambda("n", p0, apply(dot(id("n"), "method"), id("n"))), id("f"));
            })()
        })
    {
        // 型環境を使いまわして型推論をすると実質的にlet束縛で式を連結したことになってしまうが
        // 今回はシャドウも型環境の上書き禁止もないため許容する
        std::cout << "Algorithm J: " << expr->J(typeMap, env) << std::endl;
        auto t = env.newType(Type::Variable{ .depth = env.depth - 1 });
        expr->M(typeMap, env, t);
        std::cout << "Algorithm M: " << t << std::endl;
    }
}