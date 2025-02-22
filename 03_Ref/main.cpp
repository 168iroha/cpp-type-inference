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


struct TypeEnvironment;
struct Region;

/// <summary>
/// リージョン型への参照
/// </summary>
using RefRegion = std::shared_ptr<Region>;

/// <summary>
/// 識別子に対応付けられたリージョン型
/// </summary>
struct Region {
    /// <summary>
    /// 基底型
    /// </summary>
    struct Base {
        /// <summary>
        /// 参照先の識別子が定義された型環境
        /// </summary>
        TypeEnvironment* env;
    };

    /// <summary>
    /// 一時オブジェクト
    /// </summary>
    struct Temporary {};

    /// <summary>
    /// 型変数
    /// </summary>
    struct Variable {
        /// <summary>
        /// 型変数の解決結果の型
        /// </summary>
        std::optional<RefRegion> solve = std::nullopt;

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
        /// ジェネリック型の型変数のインデックス
        /// </summary>
        const std::size_t index = 0;
    };

    /// <summary>
    /// リージョン型の固有情報を示す型
    /// </summary>
    using kind_type = std::variant<Base, Temporary, Variable, Param>;

    /// <summary>
    /// リージョン型の固有情報
    /// </summary>
    kind_type kind;
};

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
    /// <para>型としての型クラス</para>
    /// <para>振る舞いとしては限定的にサブタイピングを許容した参照型</para>
    /// </summary>
    struct TypeClass {
        /// <summary>
        /// 型として扱う型クラス
        /// </summary>
        Constraints typeClasses = {};

        /// <summary>
        /// 参照先の値型についてのリージョン
        /// </summary>
        RefRegion region;
    };

    /// <summary>
    /// <para>参照型</para>
    /// <para>線型論理における指数的結合子に相当する</para>
    /// </summary>
    struct Ref {
        /// <summary>
        /// 基底型としての意味を示す型
        /// </summary>
        RefType base;

        /// <summary>
        /// 参照先の型
        /// </summary>
        RefType type;

        /// <summary>
        /// 参照先の値型についてのリージョン
        /// </summary>
        RefRegion region;
    };

    // 本来はこの他にタプルやリスト、エイリアスなどの組み込み機能を定義する


    /// <summary>
    /// 型の固有情報を示す型
    /// </summary>
    using kind_type = std::variant<Base, Function, Variable, Param, TypeClass, Ref>;

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
        else if (std::holds_alternative<Type::Ref>(this->kind)) {
            return std::get<Type::Ref>(this->kind).base->getTypeName();
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
    /// リージョン型における型変数のリスト
    /// </summary>
    std::vector<RefRegion> regionVals;

    /// <summary>
    /// 型変数valsおよびregionValsを内部で持つ型
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
/// 型から参照型を除去した解決済みの型を取得する
/// </summary>
/// <param name="type">参照型を除去する対象の型</param>
/// <returns>参照型を除去した結果の型</returns>
RefType unwrapRef(RefType type) {
    auto t = solved(type);
    while (std::holds_alternative<Type::Ref>(t->kind)) {
        auto& t2 = std::get<Type::Ref>(t->kind).type;
        t2 = solved(t2);
        t = t2;
    }
    return t;
}

/// <summary>
/// 解決済みの型を取得する
/// </summary>
/// <param name="type">チェックを行う型</param>
/// <returns>解決済みの型</returns>
RefRegion solved(RefRegion type) {
    if (std::holds_alternative<Region::Variable>(type->kind)) {
        auto& val = std::get<Region::Variable>(type->kind);
        if (val.solve) {
            // 解決結果が再適用されないように適用しておく
            return val.solve.emplace(solved(val.solve.value()));
        }
    }
    return type;
}

/// <summary>
/// 型環境で管理するコンテキスト情報付きの型情報
/// </summary>
struct TypeInfo {
    /// <summary>
    /// 型情報本体
    /// </summary>
    std::variant<RefType, Generic> type;

    /// <summary>
    /// 識別子が定義されるリージョン
    /// </summary>
    RefRegion region;
};

/// <summary>
/// 型環境で管理するコンテキスト情報付きの型情報への参照
/// </summary>
using RefTypeInfo = std::shared_ptr<TypeInfo>;

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
    std::unordered_map<std::string, RefTypeInfo> map = {};

    /// <summary>
    /// 識別子の名称から型を取り出す
    /// </summary>
    /// <param name="name">識別子の名称</param>
    /// <returns>nameに対応する型</returns>
    [[nodiscard]] std::optional<RefTypeInfo> lookup(const std::string& name) {
        if (auto itr = this->map.find(name); itr != this->map.end()) {
            return itr->second;
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
    /// コンテキスト情報付きの型の生成
    /// </summary>
    /// <param name="type">型情報</param>
    /// <param name="region">型情報が属するリージョン</param>
    /// <returns>生成した型</returns>
    [[nodiscard]] RefTypeInfo newTypeInfo(RefType type, RefRegion region) {
        return RefTypeInfo(new TypeInfo({ .type = type, .region = region }));
    }

    /// <summary>
    /// コンテキスト情報付きの型の生成
    /// </summary>
    /// <param name="type">型情報</param>
    /// <param name="region">型情報が属するリージョン</param>
    /// <returns>生成した型</returns>
    [[nodiscard]] RefTypeInfo newTypeInfo(Generic&& type, RefRegion region) {
        return RefTypeInfo(new TypeInfo({ .type = std::move(type), .region = region }));
    }

    /// <summary>
    /// リージョン型の生成
    /// </summary>
    /// <param name="kind">型の固有情報</param>
    /// <returns>生成したリージョン型</returns>
    [[nodiscard]] RefRegion newRegion(Region::kind_type&& kind) {
        return RefRegion(new Region({ .kind = std::move(kind) }));
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

    /// <summary>
    /// <para>regionが型環境に含まれるかを判定する</para>
    /// <para>実装の詳細はconvertを参照</para>
    /// </summary>
    /// <param name="region">判定対象のリージョン</param>
    /// <returns>含まれている場合はtrue, 含まれていない場合はfalse</returns>
    [[nodiscard]] bool include(RefRegion& region) {
        region = solved(region);
 
        if (std::holds_alternative<Region::Temporary>(region->kind)) {
            return true;
        }
        if (std::holds_alternative<Region::Base>(region->kind)) {
            // regionがthisのスコープに含まれているかを見る
            auto r1env = std::get<Region::Base>(region->kind).env;
            auto r2env = this;
            if (r1env->depth > r2env->depth) {
                return false;
            }
            else {
                // region2をregion1と同一階層に移動したときに型環境が一致するかを比較する
                while (r1env->depth != r2env->depth) {
                    r2env = r2env->parent;
                }
                return r1env == r2env;
            }
        }
        else {
            return false;
        }
    }
};

/// <summary>
/// 自由な型変数について型をgeneralizeする
/// </summary>
/// <param name="type">複製対象の型</param>
/// <param name="env">型環境</param>
/// <param name="vals">generalize対象の型変数のリスト</param>
/// <returns>複製結果</returns>
[[nodiscard]] std::variant<RefType, Generic> TypeEnvironment::generalize(RefType type, std::vector<RefType> vals) {
    std::vector<RefRegion> regionVals;

    struct fn {
        RefType& t;
        TypeEnvironment& e;
        std::vector<RefType>& v;
        std::vector<RefRegion>& rv;

        /// <summary>
        /// 自由なリージョン型の型変数について型をgeneralizeする
        /// </summary>
        /// <param name="type">複製対象の型</param>
        void generalizeRegion(RefRegion& region) {
            if (std::holds_alternative<Region::Variable>(region->kind)) {
                // リージョン型に解決済みの別のリージョンが存在するならば解決しておく
                auto& var = std::get<Region::Variable>(region->kind);
                if (var.solve) {
                    region = solved(var.solve.value());
                }

                // リージョン型の型変数をgeneralizeするか検査
                if (std::holds_alternative<Region::Variable>(region->kind)) {
                    auto& var2 = std::get<Region::Variable>(region->kind);

                    if (this->e.depth < var2.depth) {
                        // 自由な型変数のためgeneralizeする
                        // 一度generalize済みならばx.solveに解決結果が記録されるため、ここにはgeneralize未の場合のみしか到達しない
                        auto p = this->e.newRegion(Region::Param{ .index = this->rv.size() });
                        var2.solve = p;
                        region = p;
                        this->rv.push_back(p);
                    }
                }
            }
        }

        RefType operator()([[maybe_unused]] Type::Base& x) {
            // generalizeしない
            return this->t;
        }
        RefType operator()(Type::Function& x) {
            // 引数型と戻り値型をgeneralizeする
            auto genParamType = std::visit(fn{ .t = x.paramType, .e = this->e, .v = this->v, .rv = this->rv }, x.paramType->kind);
            if (x.paramType != genParamType) {
                x.paramType = genParamType;
            }
            auto genReturnType = std::visit(fn{ .t = x.returnType, .e = this->e, .v = this->v, .rv = this->rv }, x.returnType->kind);
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
                return std::visit(fn{ .t = this->t, .e = this->e, .v = this->v, .rv = this->rv }, this->t->kind);
            }

            if (this->e.depth < x.depth) {
                // 自由な型変数のためgeneralizeする
                // 一度generalize済みならばx.solveに解決結果が記録されるため、ここにはgeneralize未の場合のみしか到達しない
                // 導出した型変数に対する型制約は継承する
                auto p = this->e.newType(Type::Param{ .constraints = std::move(x.constraints), .index = this->v.size() });
                x.solve = p;
                this->v.push_back(p);
                return p;
            }
            else {
                // 束縛された型変数のためgeneralizeしない
                return this->t;
            }
        }
        RefType operator()([[maybe_unused]] Type::Param& x) {
            // 外のスコープから与えられた型変数もしくはgeneralize済みの型変数のためgeneralizeしない
            return this->t;
        }
        RefType operator()([[maybe_unused]] Type::TypeClass& x) {
            // リージョン型に対してgeneralizeする
            this->generalizeRegion(x.region);

            return this->t;
        }
        RefType operator()(Type::Ref& x) {
            // 参照先の型に対してgeneralizeする
            auto genType = std::visit(fn{ .t = x.type, .e = this->e, .v = this->v, .rv = this->rv }, x.type->kind);
            if (x.type != genType) {
                x.type = genType;
            }
            // リージョン型に対してgeneralizeする
            this->generalizeRegion(x.region);

            return this->t;
        }
    };

    auto t = std::visit(fn{ .t = type, .e = *this, .v = vals, .rv = regionVals }, type->kind);
    if (vals.size() > 0 || regionVals.size() > 0) {
        return Generic{ .vals = std::move(vals), .regionVals = std::move(regionVals), .type = std::move(t)};
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

    /// <summary>
    /// インスタンス化されたクラスメソッドを取得する
    /// </summary>
    /// <param name="typeMap">型表</param>
    /// <param name="env">型環境</param>
    /// <param name="methodName">クラスメソッド名(存在するかについては呼び出し元で保証する)</param>
    /// <param name="type">クラスメソッドを呼び出す型</param>
    /// <returns></returns>
    [[nodiscard]] RefType getInstantiatedMethod(TypeMap& typeMap, TypeEnvironment& env, const std::string& methodName, RefTypeInfo type);
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
        /// <summary>
        /// 参照型
        /// </summary>
        Generic ref;
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

        // 解決済みの型変数および参照型が存在すればそれを解消してから制約の適用を行う
        auto t = unwrapRef(type);

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
    else if (std::holds_alternative<Type::Ref>(this->kind)) {
        // 参照型の場合は参照型を解除してから型クラスのリストを取得する
        // Type::Variableの場合の動作に合わせるためにsolvedは適用しない
        return std::get<Type::Ref>(this->kind).type->getTypeClassList(typeMap);
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
    std::vector<RefRegion> regionVals;
    regionVals.reserve(type.regionVals.size());

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
    for (decltype(type.regionVals.size()) i = 0; i < type.regionVals.size(); ++i) {
        regionVals.push_back(this->newRegion(Region::Variable{ .depth = this->depth }));
    }

    struct fn {
        RefType t;
        TypeEnvironment& e;
        const std::vector<RefType>& v;
        const std::vector<RefType>& p;
        const std::vector<RefRegion>& rv;
        const std::vector<RefRegion>& rp;

        /// <summary>
        /// ジェネリック型のリージョン型の型変数について型をinstantiateする
        /// </summary>
        /// <param name="type">複製対象の型</param>
        RefRegion instantiateRegion(const RefRegion& region) {
            if (std::holds_alternative<Region::Param>(region->kind)) {
                auto& x = std::get<Region::Param>(region->kind);
                // ジェネリック型の型変数と等しい場合はinstantiateする
                if (x.index < this->rp.size() && this->rp[x.index] == region) {
                    return this->rv[x.index];
                }
                else {
                    return region;
                }
            }
            return region;
        }

        RefType operator()([[maybe_unused]] const Type::Base& x) {
            // instantiateしない
            return this->t;
        }
        RefType operator()(const Type::Function& x) {
            // 引数型と戻り値型をinstantiateする
            auto instParamType = std::visit(fn{ .t = x.paramType, .e = this->e, .v = this->v, .p = this->p, .rv = this->rv, .rp = this->rp }, x.paramType->kind);
            auto instReturnType = std::visit(fn{ .t = x.returnType, .e = this->e, .v = this->v, .p = this->p, .rv = this->rv, .rp = this->rp }, x.returnType->kind);

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
            // リージョン型についてのみinstantiateする
            auto instRegion = this->instantiateRegion(x.region);
            if (x.region == instRegion) {
                // instantiateされなかった場合は新規にインスタンスを生成しない
                return this->t;
            }
            else {
                // instantiateが生じた場合は新規にインスタンスを生成する
                return this->e.newType(Type::TypeClass{ .typeClasses = x.typeClasses, .region = std::move(instRegion) });
            }
        }
        RefType operator()([[maybe_unused]] const Type::Ref& x) {
            // 参照先の型をinstantiateする
            auto instType = std::visit(fn{ .t = x.type, .e = this->e, .v = this->v, .p = this->p, .rv = this->rv, .rp = this->rp }, x.type->kind);
            auto instRegion = this->instantiateRegion(x.region);
            if (x.type == instType && x.region == instRegion) {
                // instantiateされなかった場合は新規にインスタンスを生成しない
                return this->t;
            }
            else {
                // instantiateが生じた場合は新規にインスタンスを生成する
                // 組込み型である参照型なので基底を継承する
                return this->e.newType(Type::Ref{ .base = x.base, .type = std::move(instType), .region = std::move(instRegion) });
            }
        }
    };

    return std::visit(fn{ .t = type.type, .e = *this, .v = vals, .p = type.vals, .rv = regionVals, .rp = type.regionVals }, type.type->kind);
}

/// <summary>
/// <para>region2からregion1へ暗黙の型変換をする</para>
/// <para>束でいうならば型変数をtop、Region::Temporaryをbottomとしてregion2≧region1の場合に変換する</para>
/// </summary>
/// <param name="region1">変換先のリージョン</param>
/// <param name="region2">変換元のリージョン</param>
/// <returns>変換を実施した場合はtrue, 変換を実施していない場合はfalse</returns>
[[nodiscard]] bool convert(RefRegion& region1, RefRegion& region2) {
    // 型解決のネストを解消する
    region1 = solved(region1);
    region2 = solved(region2);

    if (std::holds_alternative<Region::Temporary>(region1->kind)) {
        // 変換先がbottomであるため常に変換可能
        if (std::holds_alternative<Region::Variable>(region2->kind)) {
            std::get<Region::Variable>(region2->kind).solve = region1;
        }
        region2 = region1;
        return true;
    }
    if (!std::holds_alternative<Region::Temporary>(region2->kind)) {
        if (std::holds_alternative<Region::Variable>(region2->kind)) {
            // topのため常に変換可能
            std::get<Region::Variable>(region2->kind).solve = region1;
            region2 = region1;
            return true;
        }
        else if (std::holds_alternative<Region::Variable>(region1->kind)) {
            // region2がtopならばregion1もtopである必要があるため変換不可
            return false;
        }
        else if (std::holds_alternative<Region::Param>(region1->kind) || std::holds_alternative<Region::Param>(region2->kind)) {
            // region1とregion2のどちらかがジェネリック型に出現する型変数の場合は同じリージョンでなければ変換不可
            return region1 == region2;
        }
        else {
            auto r1env = std::get<Region::Base>(region2->kind).env;
            auto r2env = std::get<Region::Base>(region1->kind).env;
            if (r1env->depth > r2env->depth) {
                // 深いスコープから浅いスコープへの変換は不可にする
                // リージョン推論などによりスコープの変換が可能な場合はその限りではない
                return false;
            }
            else {
                // region2をregion1と同一階層に移動したときに型環境が一致するかを比較する
                while (r1env->depth != r2env->depth) {
                    r2env = r2env->parent;
                }
                if (r1env == r2env) {
                    region2 = region1;
                    return true;
                }
                return false;
            }
        }
    }
    else {
        // region1がbottomならばregion2もbottomである必要があるため変換不可
        return false;
    }
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
        bool operator()(const Type::Ref& x) {
            return  x.type == this->t || depend(x.type, this->t);
        }
    };

    return type == target || std::visit(fn{ .t = target }, type->kind);
}

/// <summary>
/// 暗黙の型変換のパターンの列挙
/// </summary>
enum struct ImplicitCastPattern {
    /// <summary>
    /// 参照型への変換
    /// </summary>
    REFERENCE,
    /// <summary>
    /// 型としての型クラスへの変換
    /// </summary>
    TYPECLASS,

    /// <summary>
    /// 暗黙の型変換はなし
    /// </summary>
    NONE
};

/// <summary>
/// <para>暗黙の型変換付きの副作用付きの2つの型の単一化</para>
/// <para>暗黙の型変換はtype1 &lt;- type2の方向で行われる</para>
/// </summary>
/// <param name="typeMap">型表</param>
/// <param name="type1">単一化の対象の型1</param>
/// <param name="type2">単一化の対象の型2</param>
/// <param name="implicitCast">暗黙の型変換が有効であるか</param>
/// <returns>
/// <para>暗黙の型変換が生じたかを示すImplicitCastPatternの値</para>
/// <para>NONE以外の場合は暗黙の型変換が生じたため、明示的なキャストの構文を構文木に挿入する対処が必要</para>
/// </returns>
ImplicitCastPattern unifyType(TypeMap& typeMap, RefType& type1, RefType& type2, bool implicitCast) {

    // 解決済みの型変数が存在すればそれを適用してから単一化を行う
    type1 = solved(type1);
    type2 = solved(type2);

    if (type1 != type2) {
        if (std::holds_alternative<Type::Variable>(type1->kind)) {
            auto& t1v = std::get<Type::Variable>(type1->kind);

            if (std::holds_alternative<Type::Variable>(type2->kind)) {
                // 型変数同士の場合は型の循環が起きないように外のスコープのものを設定
                auto& t2v = std::get<Type::Variable>(type2->kind);
                if (t1v.depth < t2v.depth) {
                    // 型制約をマージする
                    t1v.constraints.merge(t2v.constraints.list);
                    t2v.solve = type1;
                    type2 = type1;
                }
                else {
                    // 型制約をマージする
                    t2v.constraints.merge(t1v.constraints.list);
                    t1v.solve = type2;
                    type1 = type2;
                }
            }
            else {
                if (depend(type1, type2)) {
                    // 再帰的な単一化は決定不能のため異常(ex. x -> x xのような関数)
                    throw std::runtime_error("再帰的単一化");
                }
                // 一方のみが型変数の場合はもう一方と型を一致させる
                // type2がtype1の制約を満たすかを検証する
                typeMap.applyConstraint(type2, t1v.constraints.list);
                t1v.solve = type2;
                type1 = type2;
            }
        }
        else {
            if (std::holds_alternative<Type::Variable>(type2->kind)) {
                auto& t2v = std::get<Type::Variable>(type2->kind);
                if (depend(type2, type1)) {
                    // 再帰的な単一化は決定不能のため異常(ex. x -> x xのような関数)
                    throw std::runtime_error("再帰的単一化");
                }
                // 一方のみが型変数の場合はもう一方と型を一致させる
                // type1がtype2の制約を満たすかを検証する
                typeMap.applyConstraint(type1, t2v.constraints.list);
                t2v.solve = type1;
                type2 = type1;
            }
            else {
                // 型が一致する場合に部分型について再帰的に単一化
                if (type1->kind.index() == type2->kind.index()) {
                    if (std::holds_alternative<Type::Function>(type1->kind)) {
                        auto& k1 = std::get<Type::Function>(type1->kind);
                        auto& k2 = std::get<Type::Function>(type2->kind);
                        // type2 <: type1を仮定すると引数型は反変、戻り値型は共変なサブタイピングが行えるが
                        // バイナリレベルでの互換性はないため暗黙の型変換は無効
                        unifyType(typeMap, k1.paramType, k2.paramType, false);
                        unifyType(typeMap, k1.returnType, k2.returnType, false);
                        if (k1.paramType == k2.paramType && k1.returnType == k2.returnType) {
                            type1 = type2;
                        }
                    }
                    else if (implicitCast && std::holds_alternative<Type::TypeClass>(type1->kind)) {
                        auto& k1 = std::get<Type::TypeClass>(type1->kind);
                        auto& k2 = std::get<Type::TypeClass>(type2->kind);

                        // 型クラス同士が一致していない場合は暗黙の型変換が実施可能か検証する
                        std::ranges::sort(k1.typeClasses.list);
                        std::ranges::sort(k2.typeClasses.list);
                        bool ret = !std::ranges::equal(k1.typeClasses.list, k2.typeClasses.list);
                        if (ret) {
                            // type1に課された型クラスがtype2で実装されているか検証
                            typeMap.applyConstraint(type2, k1.typeClasses.list);
                        }

                        // 型クラス同士の場合はtype1 <- type2でリージョンも一致させる
                        if (!convert(k1.region, k2.region)) {
                            // リージョンに互換性がない
                            assert(false);
                        }
                        if (!ret) {
                            type1 = type2;
                        }

                        return ret ? ImplicitCastPattern::TYPECLASS : ImplicitCastPattern::NONE;
                    }
                    else if (std::holds_alternative<Type::Ref>(type1->kind)) {
                        auto& k1 = std::get<Type::Ref>(type1->kind);
                        auto& k2 = std::get<Type::Ref>(type2->kind);
                        // 参照型同士では暗黙の型変換を無効
                        unifyType(typeMap, k1.type, k2.type, false);

                        // 参照型同士の場合はtype1 <- type2でリージョンも一致させる
                        if (!convert(k1.region, k2.region)) {
                            // リージョンに互換性がない
                            assert(false);
                        }
                        if (k1.type == k2.type) {
                            type1 = type2;
                        }
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
    return ImplicitCastPattern::NONE;
}

/// <summary>
/// <para>参照型への暗黙の型変換付きの副作用付きの2つの型の単一化</para>
/// <para>暗黙の型変換はtype1 &lt;- type2の方向で行われる</para>
/// </summary>
/// <param name="typeMap">型表</param>
/// <param name="type1">単一化の対象の型1</param>
/// <param name="type2">単一化の対象の型2</param>
/// <returns>
/// <para>暗黙の型変換が生じたかを示すImplicitCastPatternの値</para>
/// <para>NONE以外の場合は暗黙の型変換が生じたため、明示的なキャストの構文を構文木に挿入する対処が必要</para>
/// </returns>
ImplicitCastPattern unifyWithRef(TypeMap& typeMap, RefType& type1, RefTypeInfo type2) {
    // ジェネリック型を単一化に含まないことは論理エラーとする
    assert(std::holds_alternative<RefType>(type2->type));

    // 解決済みの型変数が存在すればそれを適用してから単一化を行う
    type1 = solved(type1);
    auto& t2 = std::get<RefType>(type2->type);
    t2 = solved(t2);

    if (type1->kind.index() != t2->kind.index() && !std::holds_alternative<Type::Variable>(t2->kind)) {
        if (std::holds_alternative<Type::TypeClass>(type1->kind)) {
            // type1 <- type2な型としての型クラスへの暗黙の型変換の検証
            // type1に課された型クラスがtype2で実装されているか検証となる
            auto& k1 = std::get<Type::TypeClass>(type1->kind);
            typeMap.applyConstraint(t2, k1.typeClasses.list);

            // 暗黙の型変換に成功したならばtype1の参照先のリージョンにtype2のリージョンを反映
            if (!convert(type2->region, k1.region)) {
                // リージョンに互換性がない
                assert(false);
            }

            return ImplicitCastPattern::TYPECLASS;
        }
        else if (std::holds_alternative<Type::Ref>(type1->kind)) {
            // type1 <- type2な参照型ではない型から参照型への暗黙の型変換の検証
            // 参照型の参照先の型との暗黙の型変換は無効化する
            auto& k1 = std::get<Type::Ref>(type1->kind);
            unifyType(typeMap, k1.type, t2, false);

            // 暗黙の型変換に成功したならばtype1の参照先のリージョンにtype2のリージョンを反映
            if (!convert(type2->region, k1.region)) {
                // リージョンに互換性がない
                assert(false);
            }

            return ImplicitCastPattern::REFERENCE;
        }
    }

    // 参照型に関する暗黙の型変換が生じないパターンでは通常の単一化を行う
    return unifyType(typeMap, type1, t2, true);
}

/// <summary>
/// <para>暗黙の型変換付きの副作用付きの2つの関数型の単一化</para>
/// <para>暗黙の型変換は引数型と戻り値型でそれぞれtype1 &lt;- type2の方向で行われる</para>
/// </summary>
/// <param name="typeMap">型表</param>
/// <param name="env">型環境</param>
/// <param name="type1">単一化の対象の型1</param>
/// <param name="type2p">単一化の対象の型2の引数型</param>
/// <param name="type2r">単一化の対象の型2の戻り値型</param>
/// <returns>
/// <para>暗黙の型変換が生じたかを示すImplicitCastPatternの値</para>
/// <para>NONE以外の場合は暗黙の型変換が生じたため、明示的なキャストの構文を構文木に挿入する対処が必要</para>
/// </returns>
std::variant<std::pair<ImplicitCastPattern, ImplicitCastPattern>, ImplicitCastPattern> unifyFunction(TypeMap& typeMap, TypeEnvironment& env, RefType type1, RefTypeInfo type2p, RefTypeInfo type2r) {

    // 解決済みの型変数が存在すればそれを適用してから単一化を行う
    auto t1 = solved(type1);

    if (std::holds_alternative<Type::Function>(t1->kind)) {
        auto& k1 = std::get<Type::Function>(t1->kind);
        return std::pair{
            unifyWithRef(typeMap, k1.paramType, type2p),
            unifyWithRef(typeMap, k1.returnType, type2r)
        };
    }
    else {
        // 関数型の引数型と戻り値型に関して個別の単一化ができない場合は通常の単一化を行う
        // type1は型変数でなければ異常となる
        if (std::holds_alternative<Type::Variable>(t1->kind)) {
            std::get<Type::Variable>(t1->kind).solve = env.instantiate(typeMap, typeMap.builtin.fn, { std::get<RefType>(type2p->type), std::get<RefType>(type2r->type) });
        }
        else {
            // 型の種類が一致しない
            throw std::runtime_error("型の不一致");
        }
        return ImplicitCastPattern::NONE;
    }
}

/// <summary>
/// インスタンス化されたクラスメソッドを取得する
/// </summary>
/// <param name="typeMap">型表</param>
/// <param name="env">型環境</param>
/// <param name="methodName">クラスメソッド名(存在するかについては呼び出し元で保証する)</param>
/// <param name="type">クラスメソッドを呼び出す型</param>
/// <returns></returns>
[[nodiscard]] RefType TypeClass::getInstantiatedMethod(TypeMap& typeMap, TypeEnvironment& env, const std::string& methodName, RefTypeInfo type) {
    assert(this->methods.contains(methodName));

    // クラスメソッドがジェネリック型ならinstantiateする
    // 型クラスを実装する対象の型についてもinstantiateする
    auto& method = this->methods.at(methodName);
    auto f = env.instantiate(typeMap, Generic{
        .vals = { this->type },
        .regionVals = {},
        .type = (
            std::holds_alternative<Generic>(method) ?
            env.instantiate(typeMap, std::get<Generic>(method)) :
            std::get<RefType>(method)
        )
        });

    // 第一引数がtypeで呼び出し可能なクラスメソッドであるかを検査する
    // 例えば、typeが参照型でfの第一引数が値型の場合は呼び出し不可
    unifyWithRef(typeMap, std::get<Type::Function>(f->kind).paramType, type);

    return std::get<Type::Function>(f->kind).returnType;
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
    virtual RefTypeInfo J(TypeMap& typeMap, TypeEnvironment& env) = 0;

    /// <summary>
    /// Algorithm Mの適用
    /// </summary>
    /// <param name="typeMap">型表</param>
    /// <param name="env">型環境</param>
    /// <param name="rho">式が推測される型</param>
    virtual void M(TypeMap& typeMap, TypeEnvironment& env, RefTypeInfo rho) = 0;
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
    RefTypeInfo J([[maybe_unused]] TypeMap& typeMap, TypeEnvironment& env) override {
        return env.newTypeInfo(this->b, env.newRegion(Region::Temporary{}));
    }

    /// <summary>
    /// Algorithm Mの適用
    /// </summary>
    /// <param name="typeMap">型表</param>
    /// <param name="env">型環境</param>
    /// <param name="rho">式が推測される型</param>
    void M(TypeMap& typeMap, [[maybe_unused]] TypeEnvironment& env, RefTypeInfo rho) override {
        // リテラルのインスタンスは常に一時オブジェクトとして扱う
        unifyWithRef(typeMap, std::get<RefType>(rho->type), env.newTypeInfo(this->b, env.newRegion(Region::Temporary{})));
        rho->region->kind = Region::Temporary{};
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
    RefTypeInfo J(TypeMap& typeMap, TypeEnvironment& env) override {
        // 型環境から型を取り出す
        auto tau = env.lookup(this->x);
        if (tau) {
            // 使用済みであるかの検証は線型型をを未実装のためまだない
            
            auto& type = tau.value()->type;
            if (std::holds_alternative<RefType>(type)) {
                return tau.value();
            }
            else {
                // 多相のためにinstantiateする(単相の場合は不要)
                // ジェネリック型のインスタンスは常に一時オブジェクトとして扱う
                return env.newTypeInfo(env.instantiate(typeMap, std::get<Generic>(type)), env.newRegion(Region::Temporary{}));
            }
        }
        throw std::runtime_error(std::format("不明な識別子：{}", this->x));
    }

    /// <summary>
    /// Algorithm Mの適用
    /// </summary>
    /// <param name="typeMap">型表</param>
    /// <param name="env">型環境</param>
    /// <param name="rho">式が推測される型</param>
    void M(TypeMap& typeMap, TypeEnvironment& env, RefTypeInfo rho) override {
        // 型環境から型を取り出す
        auto tau = env.lookup(this->x);
        if (tau) {
            // 使用済みであるかの検証は線型型をを未実装のためまだない

            auto& type = tau.value()->type;
            if (std::holds_alternative<RefType>(type)) {
                if (unifyWithRef(typeMap, std::get<RefType>(rho->type), tau.value()) == ImplicitCastPattern::NONE) {
                    // 暗黙的な型変換が生じなかった場合はtauと同じリージョンをもたせる
                    // rhoで与えられるリージョン型は基本的にenvと同じ型環境上の型変数と仮定しているため、convertによる変換は不可
                    rho->region = tau.value()->region;
                }
                else {
                    // 暗黙的な型変換が生じた場合は一時オブジェクトとして扱う
                    rho->region->kind = Region::Temporary{};
                }
            }
            else {
                // 多相のためにinstantiateする(単相の場合は不要)
                // ジェネリック型のインスタンスは常に一時オブジェクトとして扱う
                unifyWithRef(typeMap, std::get<RefType>(rho->type), env.newTypeInfo(env.instantiate(typeMap, std::get<Generic>(type)), env.newRegion(Region::Temporary{})));
                rho->region->kind = Region::Temporary{};
            }
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
    /// typeについてラムダ抽象に関するダングリングが生じているかをチェックする
    /// </summary>
    /// <param name="env">ラムダ抽象のために構成された型環境(リージョン)</param>
    /// <param name="type">ラムダ抽象結果のジェネリックではない戻り値型情報</param>
    /// <returns>ダングリングが生じている場合はtrue、ダングリングが生じていない場合はfalse</returns>
    static bool checkDangling(TypeEnvironment& env, RefTypeInfo type) {
        // 戻り値型が参照型かつをenvに属しているかを判定する
        auto& t = std::get<RefType>(type->type);
        t = solved(t);
        return std::holds_alternative<Type::Ref>(t->kind) && env.include(std::get<Type::Ref>(t->kind).region);
    }

    /// <summary>
    /// Algorithm Jの適用
    /// </summary>
    /// <param name="typeMap">型表</param>
    /// <param name="env">型環境</param>
    /// <returns>評価結果の型</returns>
    RefTypeInfo J(TypeMap& typeMap, TypeEnvironment& env) override {
        // 型環境を新しく構成
        // 一引数単位で型環境を構築するのは効率が悪いため通常は複数の引数を一度に扱う
        TypeEnvironment newEnv = {
            .parent = std::addressof(env),
            .depth = env.depth + 1
        };

        // 型環境にxを登録してeを評価
        auto t = env.newTypeInfo(
            this->constraint ? this->constraint.value() : newEnv.newType(Type::Variable{ .depth = newEnv.depth }),
            env.newRegion(Region::Base{ .env = std::addressof(newEnv)})
        );
        newEnv.map.insert({ this->x, t });
        auto tau = this->e->J(typeMap, newEnv);

        auto ret = env.newTypeInfo(env.instantiate(typeMap, typeMap.builtin.fn, { std::get<RefType>(t->type), std::get<RefType>(tau->type) }), env.newRegion(Region::Temporary{}));

        if (Lambda::checkDangling(newEnv, ret)) {
            throw std::runtime_error("ダングリング");
        }

        return ret;
    }

    /// <summary>
    /// Algorithm Mの適用
    /// </summary>
    /// <param name="typeMap">型表</param>
    /// <param name="env">型環境</param>
    /// <param name="rho">式が推測される型</param>
    void M(TypeMap& typeMap, TypeEnvironment& env, RefTypeInfo rho) override {
        // 型環境を新しく構成
        // 一引数単位で型環境を構築するのは効率が悪いため通常は複数の引数を一度に扱う
        TypeEnvironment newEnv = {
            .parent = std::addressof(env),
            .depth = env.depth + 1
        };

        auto t1 = newEnv.newTypeInfo(
            this->constraint ? this->constraint.value() : newEnv.newType(Type::Variable{ .depth = newEnv.depth }),
            newEnv.newRegion(Region::Base{ .env = std::addressof(newEnv) })
        );
        auto t2 = newEnv.newTypeInfo(newEnv.newType(Type::Variable{ .depth = newEnv.depth }), newEnv.newRegion(Region::Variable{ .depth = newEnv.depth }));
        unifyFunction(typeMap, env, std::get<RefType>(rho->type), t1, t2);

        // 型環境にxを登録してeを評価
        newEnv.map.insert({ this->x, t1 });
        this->e->M(typeMap, newEnv, t2);

        if (Lambda::checkDangling(newEnv, t2)) {
            throw std::runtime_error("ダングリング");
        }
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
    RefTypeInfo J(TypeMap& typeMap, TypeEnvironment& env) override {
        auto tau1 = this->e1->J(typeMap, env);
        auto tau2 = this->e2->J(typeMap, env);
        auto t = env.newTypeInfo(env.newType(Type::Variable{ .depth = env.depth }), env.newRegion(Region::Temporary{}));

        unifyFunction(typeMap, env, std::get<RefType>(tau1->type), tau2, t);

        return t;
    }

    /// <summary>
    /// Algorithm Mの適用
    /// </summary>
    /// <param name="typeMap">型表</param>
    /// <param name="env">型環境</param>
    /// <param name="rho">式が推測される型</param>
    void M(TypeMap& typeMap, TypeEnvironment& env, RefTypeInfo rho) override {
        auto t = env.newTypeInfo(env.newType(Type::Variable{ .depth = env.depth }), env.newRegion(Region::Base{ .env = std::addressof(env) }));

        this->e1->M(typeMap, env, env.newTypeInfo(env.instantiate(typeMap, typeMap.builtin.fn, { std::get<RefType>(t->type), std::get<RefType>(rho->type) }), env.newRegion(Region::Base{ .env = std::addressof(env) })));
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
    /// typeについてlet束縛に関するダングリングが生じているかをチェックする
    /// </summary>
    /// <param name="type">let束縛するジェネリックではない型情報</param>
    /// <returns>ダングリングが生じている場合はtrue、ダングリングが生じていない場合はfalse</returns>
    static bool checkDangling(RefTypeInfo type) {
        // 一時オブジェクトへの参照をlet束縛しているかを判定する
        auto& t = std::get<RefType>(type->type);
        t = solved(t);
        return std::holds_alternative<Type::Ref>(t->kind) && std::holds_alternative<Region::Temporary>(solved(std::get<Type::Ref>(t->kind).region)->kind);
    }

    /// <summary>
    /// Algorithm Jの適用
    /// </summary>
    /// <param name="typeMap">型表</param>
    /// <param name="env">型環境</param>
    /// <returns>評価結果の型</returns>
    RefTypeInfo J(TypeMap& typeMap, TypeEnvironment& env) override {
        auto tau1 = this->e1->J(typeMap, env);

        if (Let::checkDangling(tau1)) {
            throw std::runtime_error(std::format("ダングリング：{}", this->x));
        }

        // 識別子の多重定義の禁止
        if (env.map.contains(this->x)) {
            throw std::runtime_error(std::format("識別子が同一スコープで多重定義されている：{}", this->x));
        }
        // 型環境にxを定義
        auto g = env.generalize(std::get<RefType>(tau1->type), this->params);
        auto region = env.newRegion(Region::Base{ .env = std::addressof(env) });
        auto typeInfo = std::holds_alternative<Generic>(g) ? env.newTypeInfo(std::move(std::get<Generic>(g)), region) : env.newTypeInfo(std::get<RefType>(g), region);
        env.map.insert({ this->x,  typeInfo });

        return this->e2->J(typeMap, env);
    }

    /// <summary>
    /// Algorithm Mの適用
    /// </summary>
    /// <param name="typeMap">型表</param>
    /// <param name="env">型環境</param>
    /// <param name="rho">式が推測される型</param>
    void M(TypeMap& typeMap, TypeEnvironment& env, RefTypeInfo rho) override {
        auto t = env.newTypeInfo(env.newType(Type::Variable{ .depth = env.depth }), env.newRegion(Region::Base{ .env = std::addressof(env) }));

        this->e1->M(typeMap, env, t);

        if (Let::checkDangling(t)) {
            throw std::runtime_error(std::format("ダングリング：{}", this->x));
        }

        // 識別子の多重定義の禁止
        if (env.map.contains(this->x)) {
            throw std::runtime_error(std::format("識別子が同一スコープで多重定義されている：{}", this->x));
        }
        // 型環境にxを定義
        auto g = env.generalize(std::get<RefType>(t->type), this->params);
        auto region = env.newRegion(Region::Base{ .env = std::addressof(env) });
        auto typeInfo = std::holds_alternative<Generic>(g) ? env.newTypeInfo(std::move(std::get<Generic>(g)), region) : env.newTypeInfo(std::get<RefType>(g), region);
        env.map.insert({ this->x,  typeInfo });

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
    RefTypeInfo J(TypeMap& typeMap, TypeEnvironment& env) override {
        // 識別子の多重定義の禁止
        if (env.map.contains(this->x)) {
            throw std::runtime_error(std::format("識別子が同一スコープで多重定義されている：{}", this->x));
        }
        // 型環境にxを定義
        auto t = env.newTypeInfo(env.newType(Type::Variable{ .depth = env.depth }), env.newRegion(Region::Base{ .env = std::addressof(env) }));
        env.map.insert({ this->x,  t });

        auto tau1 = this->e1->J(typeMap, env);
        // tau1は一時オブジェクトのためリージョン型については単一化をしない
        unifyType(typeMap, std::get<RefType>(t->type), std::get<RefType>(tau1->type), true);

        if (Let::checkDangling(t)) {
            throw std::runtime_error(std::format("ダングリング：{}", this->x));
        }

        t->type = env.generalize(std::get<RefType>(tau1->type), this->params);

        return this->e2->J(typeMap, env);
    }

    /// <summary>
    /// Algorithm Mの適用
    /// </summary>
    /// <param name="typeMap">型表</param>
    /// <param name="env">型環境</param>
    /// <param name="rho">式が推測される型</param>
    void M(TypeMap& typeMap, TypeEnvironment& env, RefTypeInfo rho) override {
        // 識別子の多重定義の禁止
        if (env.map.contains(this->x)) {
            throw std::runtime_error(std::format("識別子が同一スコープで多重定義されている：{}", this->x));
        }
        // 型環境にxを定義
        auto t1 = env.newTypeInfo(env.newType(Type::Variable{ .depth = env.depth }), env.newRegion(Region::Base{ .env = std::addressof(env) }));
        auto t2 = env.newTypeInfo(env.newType(Type::Variable{ .depth = env.depth }), env.newRegion(Region::Temporary{}));
        env.map.insert({ this->x,  t1 });

        this->e1->M(typeMap, env, t2);
        // t2は一時オブジェクトのためリージョン型については単一化をしない
        unifyType(typeMap, std::get<RefType>(t1->type), std::get<RefType>(t2->type), true);

        if (Let::checkDangling(t1)) {
            throw std::runtime_error(std::format("ダングリング：{}", this->x));
        }

        t1->type = env.generalize(std::get<RefType>(t1->type), this->params);

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
    RefTypeInfo getClassMethod(TypeMap& typeMap, TypeEnvironment& env, RefTypeInfo type) {
        // typeがxをクラスメソッドとしてただ1つもつか検査
        // 型クラスを実装しているかだけを見るため、クラスメソッドの実装方式などは見ない
        auto& typeClassList = std::get<RefType>(type->type)->getTypeClassList(typeMap);
        auto [typeClass, index] = typeClassList.getClassMethod(this->x);

        if (typeClass) {
            return env.newTypeInfo(
                typeClass.value()->getInstantiatedMethod(typeMap, env, this->x, type),
                env.newRegion(Region::Temporary{})
            );
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
    RefTypeInfo J(TypeMap& typeMap, TypeEnvironment& env) override {
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
    void M(TypeMap& typeMap, TypeEnvironment& env, RefTypeInfo rho) override {
        auto t = env.newTypeInfo(env.newType(Type::Variable{ .depth = env.depth }), env.newRegion(Region::Variable{ .depth = env.depth }));
        this->e->M(typeMap, env, t);

        // クラスメソッドを取得して部分適用結果の型を取得する
        // クラスメソッドは常に一時オブジェクトとして扱う
        unifyWithRef(typeMap, std::get<RefType>(rho->type), this->getClassMethod(typeMap, env, t));
        rho->region->kind = Region::Temporary{};
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
    /// <param name="type">クラスメソッドを実装した型</param>
    /// <returns>クラスメソッドを示す型</returns>
    RefType getClassMethod(TypeMap& typeMap, TypeEnvironment& env, RefTypeInfo type) {
        auto& typeClass = this->getTypeClass();
        auto& methodName = this->getMethodName();
        // クラスメソッドがないのは論理エラーとする
        assert(typeClass->methods.contains(methodName));

        return typeClass->getInstantiatedMethod(typeMap, env, methodName, type);
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
    RefTypeInfo J(TypeMap& typeMap, TypeEnvironment& env) override {
        // 左項については型制約の適用・検査
        auto tau1 = this->lhs->J(typeMap, env);
        typeMap.applyConstraint(std::get<RefType>(tau1->type), { this->getTypeClass() });
        auto tau2 = this->rhs->J(typeMap, env);

        // クラスメソッドに対して単一化を行って二項演算の結果型を得る
        auto t = env.newTypeInfo(env.newType(Type::Variable{ .depth = env.depth }), env.newRegion(Region::Temporary{}));
        unifyFunction(typeMap, env, this->getClassMethod(typeMap, env, tau1), tau2, t);

        return t;
    }

    /// <summary>
    /// Algorithm Mの適用
    /// </summary>
    /// <param name="typeMap">型表</param>
    /// <param name="env">型環境</param>
    /// <param name="rho">式が推測される型</param>
    void M(TypeMap& typeMap, TypeEnvironment& env, RefTypeInfo rho) override {
        auto t1 = env.newTypeInfo(env.newType(Type::Variable{ .depth = env.depth }), env.newRegion(Region::Variable{ .depth = env.depth }));
        // 左項については型制約の適用・検査
        this->lhs->M(typeMap, env, t1);
        typeMap.applyConstraint(std::get<RefType>(t1->type), { this->getTypeClass() });

        // クラスメソッドに対して単一化を行って二項演算の結果型を得る
        auto t2 = env.newTypeInfo(env.newType(Type::Variable{ .depth = env.depth }), env.newRegion(Region::Variable{ .depth = env.depth }));
        unifyFunction(typeMap, env, this->getClassMethod(typeMap, env, t1), t2, rho);

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
        std::unordered_map<const Type::Variable*, char> varmap = {};
        std::unordered_map<const Region*, char> regionmap = {};

        /// <summary>
        /// リージョン型の型変数について出力する
        /// </summary>
        /// <param name="type">出力の型</param>
        void printRegion(RefRegion region) {
            auto r = solved(region).get();
            // 一時オブジェクトは⊥で出力
            if (std::holds_alternative<Region::Temporary>(r->kind)) {
                this->o << " at ⊥";
                return;
            }
            // 雑に[a, z]の範囲でリージョン型変数を出力
            // zを超えたら全て「_」で出力
            if (auto itr = this->regionmap.find(r); itr != this->regionmap.end()) {
                this->o << " at " << itr->second;
            }
            else {
                char c = this->regionmap.size() > 'z' - 'a' ? '_' : this->regionmap.size() + 'a';
                this->regionmap.insert({ r, c });
                this->o << " at " << c;
            }
        }

        void operator()(const Type::Base& x) {
            this->o << x.name;
        }
        void operator()(const Type::Function& x) {
            // 括弧付きで出力するかを判定
            bool isSimple = std::holds_alternative<Type::Base>(x.paramType->kind) ||
                std::holds_alternative<Type::Variable>(x.paramType->kind) ||
                std::holds_alternative<Type::Param>(x.paramType->kind) ||
                std::holds_alternative<Type::TypeClass>(x.paramType->kind) ||
                std::holds_alternative<Type::Ref>(x.paramType->kind);
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
                std::visit(*this, solved(x.solve.value())->kind);
            }
            else {
                // 雑に[a, z]の範囲で型変数を出力
                // zを超えたら全て「_」で出力
                if (auto itr = this->varmap.find(std::addressof(x)); itr != this->varmap.end()) {
                    this->o << '?' << itr->second;
                }
                else {
                    char c = this->varmap.size() > 'z' - 'a' ? '_' : this->varmap.size() + 'a';
                    this->varmap.insert({ std::addressof(x), c });
                    this->o << '?' << c;
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
            this->printRegion(x.region);
        }
        void operator()(const Type::Ref& x) {
            std::visit(*this, x.type->kind);
            this->o << '&';
            this->printRegion(x.region);
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
RefType fun(TypeMap& typeMap, TypeEnvironment& env, RefType paramType, RefType returnType) { return env.instantiate(typeMap, typeMap.builtin.fn, { paramType, returnType }); }
template <class... Types>
RefType tc(TypeEnvironment& env, Types&&... args) { return env.newType(Type::TypeClass{ .typeClasses{.list = { std::forward<Types>(args)...}}, .region = env.newRegion(Region::Variable{.depth = env.depth + 1 }) }); }
RefType ref(TypeEnvironment& env, RefType base, RefType type) { return env.newType(Type::Ref{ .base = base, .type = type, .region = env.newRegion(Region::Variable{.depth = env.depth + 1 }) }); }
RefType ref(TypeMap& typeMap, TypeEnvironment& env, RefType type) { return env.instantiate(typeMap, typeMap.builtin.ref, { type }); }
RefTypeInfo info(TypeEnvironment& env, RefType type) { return env.newTypeInfo(type, env.newRegion(Region::Base{ .env = std::addressof(env) })); }
RefTypeInfo info(TypeEnvironment& env, Generic&& type) { return env.newTypeInfo(std::move(type), env.newRegion(Region::Base{ .env = std::addressof(env) })); }

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

    // 適当に数値型とBoolean型、関数、参照型を定義しておく
    typeMap.builtin.fn = std::get<Generic>(env.generalize(fun(env, base(env, "fn"), var(env), var(env))));
    typeMap.addType(typeMap.builtin.fn);
    typeMap.builtin.ref = std::get<Generic>(env.generalize(ref(env, base(env, "ref"), var(env))));
    typeMap.addType(typeMap.builtin.ref);
    auto& [numberN, numberTD] = typeMap.addType(base(env, "number"));
    auto& numberT = std::get<RefType>(numberTD.type);
    auto& [booleanN, booleanTD] = typeMap.addType(base(env, "boolean"));
    auto& booleanT = std::get<RefType>(booleanTD.type);

    // 適当に型クラスを定義する
    typeMap.addTypeClass(([&typeMap, &env] {
        auto valT = param(env);
        return RefTypeClass(new TypeClass({
            .name = "TypeClass",
            .type = valT,
            .methods = {
                // method :: 'a -> 'a -> 'a
                { "method", fun(typeMap, env, valT, fun(typeMap, env, valT, valT)) }
            }
        }));
    })());
    // Boolean型に型クラスTypeClassを実装する
    booleanTD.typeclasses.list.push_back(typeMap.typeClassMap["TypeClass"]);

    // 定数のつもりの構文を宣言しておく
    auto _true = c(booleanT);
    auto _1 = c(numberT);

    for (auto& expr :
        {
            // let f = n: (:TypeClass) -> n.method n in f
            // 引数型に型を明示的に指定してクラスメソッドを呼び出す例
            // 型としての型クラスは参照型の一形態のためリージョン情報も出力される
            let("f", lambda("n", tc(env, typeMap.typeClassMap["TypeClass"]), apply(dot(id("n"), "method"), id("n"))), apply(id("f"), _true)),
            // let g = n: 'a& at a -> 1 in g true
            // 暗黙の型推論により値型から参照型へ変換される例
            let("g", lambda("n", ref(typeMap, env, var(env)), _1), apply(id("g"), _true)),
            // let h = n: 'a& at a ->'a& at a in (let i = h true in i)
            // 一時オブジェクトへの参照をlet束縛しようとしてダングリングが生じる例
            let("h", lambda("n", ref(typeMap, env, var(env)), id("n")), let("i", apply(id("h"), _true), id("i")))
        })
    {
        try {
            // 型環境の使いまわしは不可のためAlgorithm JとAlgorithm Mの両方を同時に動かすことは不可
            std::cout << std::get<RefType>(expr->J(typeMap, env)->type) << std::endl;
            //auto t = env.newTypeInfo(env.newType(Type::Variable{ .depth = env.depth }), env.newRegion(Region::Variable{ .depth = env.depth }));
            //expr->M(typeMap, env, t);
            //std::cout << std::get<RefType>(t->type) << std::endl;
        }
        catch (const std::runtime_error& e) {
            std::cout << e.what() << std::endl;
        }
    }
}