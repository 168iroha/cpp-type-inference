#include <string>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <format>
#include <memory>
#include <variant>
#include <type_traits>
#include <optional>

#include <iostream>

struct Type;

/// <summary>
/// 型への参照
/// </summary>
using RefType = std::shared_ptr<Type>;

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
        // 型制約については未実装

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
        // 型制約については未実装

        /// <summary>
        /// ジェネリック型の型変数のインデックス
        /// </summary>
        const std::size_t index = 0;
    };

    // 本来はこの他にタプルやリスト、エイリアスなどの組み込み機能を定義する


    /// <summary>
    /// 型の固有情報を示す型
    /// </summary>
    using kind_type = std::variant<Base, Function, Variable, Param>;

    /// <summary>
    /// 型の固有情報
    /// </summary>
    kind_type kind;
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
    /// <returns>複製結果</returns>
    [[nodiscard]] std::variant<RefType, Generic> generalize(RefType type);

    /// <summary>
    /// ジェネリック型の型変数についてinstantiateする
    /// </summary>
    /// <param name="type">複製対象の型</param>
    /// <returns>複製結果</returns>
    [[nodiscard]] RefType instantiate(Generic type);
};

/// <summary>
/// 自由な型変数について型をgeneralizeする
/// </summary>
/// <param name="type">複製対象の型</param>
/// <param name="env">型環境</param>
/// <returns>複製結果</returns>
[[nodiscard]] std::variant<RefType, Generic> TypeEnvironment::generalize(RefType type) {
    // generalizeした対象の型変数のリスト
    std::vector<RefType> vals;
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
                    this->v.push_back(this->e.newType(Type::Param{ .index = this->v.size() }));
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
/// ジェネリック型の型変数についてinstantiateする
/// </summary>
/// <param name="type">複製対象の型</param>
/// <returns>複製結果</returns>
[[nodiscard]] RefType TypeEnvironment::instantiate(Generic type) {
    // instantiateした対象の型変数のリスト
    std::vector<RefType> vals(type.vals.size());

    struct fn {
        RefType t;
        TypeEnvironment& e;
        std::vector<RefType>& v;
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
                return this->e.newType(Type::Function{ .paramType = std::move(instParamType), .returnType = std::move(instReturnType) });
            }
        }
        RefType operator()([[maybe_unused]] const Type::Variable& x) {
            // 外のスコープから与えられた型変数のためinstantiateしない
            return this->t;
        }
        RefType operator()(const Type::Param& x) {
            // ジェネリック型の型変数と等しい場合はinstantiateする
            if (x.index < this->p.size() && this->p[x.index] == this->t) {
                if (this->v[x.index]) {
                    // 既に型変数を割り当て済みの場合はそこから持ってくる
                    return this->v[x.index];
                }
                else {
                    auto type = this->e.newType(Type::Variable{ .depth = this->e.depth });
                    return this->v[x.index] = type;
                }
            }
            else {
                return this->t;
            }
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
    };

    return type == target || std::visit(fn{ .t = target }, type->kind);
}

/// <summary>
/// 副作用付きの2つの型の単一化
/// </summary>
/// <param name="type1">単一化の対象の型1</param>
/// <param name="type2">単一化の対象の型2</param>
void unify(RefType type1, RefType type2) {

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
                    t2v.solve = t1;
                }
                else {
                    t1v.solve = t2;
                }
            }
            else {
                if (depend(t1, t2)) {
                    // 再帰的な単一化は決定不能のため異常(ex. x -> x xのような関数)
                    throw std::runtime_error("再帰的単一化");
                }
                // 一方のみが型変数の場合はもう一方と型を一致させる
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
                t2v.solve = t1;
            }
            else {
                // 型が一致する場合に部分型について再帰的に単一化
                // 制約付きの場合は「C ⊨ type1 ∼ type2」のような同値の条件によりキャストを行うが未実装
                if (t1->kind.index() == t2->kind.index()) {
                    if (std::holds_alternative<Type::Function>(t1->kind)) {
                        auto& k1 = std::get<Type::Function>(t1->kind);
                        auto& k2 = std::get<Type::Function>(t2->kind);
                        unify(k1.paramType, k2.paramType);
                        unify(k1.returnType, k2.returnType);
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
    /// <param name="env">型環境</param>
    /// <returns>評価結果の型</returns>
    virtual RefType J(TypeEnvironment& env) = 0;

    /// <summary>
    /// Algorithm Mの適用
    /// </summary>
    /// <param name="env">型環境</param>
    /// <param name="rho">式が推測される型</param>
    virtual void M(TypeEnvironment& env, RefType rho) = 0;
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
    /// <param name="env">型環境</param>
    /// <returns>評価結果の型</returns>
    RefType J([[maybe_unused]] TypeEnvironment& env) override {
        return this->b;
    }

    /// <summary>
    /// Algorithm Mの適用
    /// </summary>
    /// <param name="env">型環境</param>
    /// <param name="rho">式が推測される型</param>
    void M(TypeEnvironment& env, RefType rho) override {
        unify(rho, this->b);
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
    /// <param name="env">型環境</param>
    /// <returns>評価結果の型</returns>
    RefType J(TypeEnvironment& env) override {
        struct fn {
            TypeEnvironment& e;

            RefType operator()(const RefType& x) {
                return x;
            }
            RefType operator()(const Generic& x) {
                // 多相のためにinstantiateする(単相の場合は不要)
                return this->e.instantiate(x);
            }
        };

        // 型環境から型を取り出す
        auto tau = env.lookup(this->x);
        if (tau) {
            return std::visit(fn{ .e = env }, *tau.value());
        }
        throw std::runtime_error(std::format("不明な識別子：{}", this->x));
    }

    /// <summary>
    /// Algorithm Mの適用
    /// </summary>
    /// <param name="env">型環境</param>
    /// <param name="rho">式が推測される型</param>
    void M(TypeEnvironment& env, RefType rho) override {
        struct fn {
            TypeEnvironment& e;

            RefType operator()(const RefType& x) {
                return x;
            }
            RefType operator()(const Generic& x) {
                // 多相のためにinstantiateする(単相の場合は不要)
                return this->e.instantiate(x);
            }
        };

        // 型環境から型を取り出す
        auto tau = env.lookup(this->x);

        if (tau) {
            unify(rho, std::visit(fn{ .e = env }, *tau.value()));
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
    /// 関数本体の式
    /// </summary>
    std::shared_ptr<Expression> e;

    Lambda(std::string_view x, std::shared_ptr<Expression> e) : x(x), e(e) {}
    ~Lambda() override {}

    /// <summary>
    /// Algorithm Jの適用
    /// </summary>
    /// <param name="env">型環境</param>
    /// <returns>評価結果の型</returns>
    RefType J(TypeEnvironment& env) override {
        // 型環境を新しく構成
        // 一引数単位で型環境を構築するのは効率が悪いため通常は複数の引数を一度に扱う
        TypeEnvironment newEnv = {
            .parent = std::addressof(env),
            .depth = env.depth + 1
        };

        // 型環境にxを登録してeを評価
        auto t = newEnv.newType(Type::Variable{ .depth = newEnv.depth });
        newEnv.map.insert({ this->x, t });
        auto tau = this->e->J(newEnv);

        return env.newType(Type::Function{ .paramType = t, .returnType = tau });
    }

    /// <summary>
    /// Algorithm Mの適用
    /// </summary>
    /// <param name="env">型環境</param>
    /// <param name="rho">式が推測される型</param>
    void M(TypeEnvironment& env, RefType rho) override {
        // 型環境を新しく構成
        // 一引数単位で型環境を構築するのは効率が悪いため通常は複数の引数を一度に扱う
        TypeEnvironment newEnv = {
            .parent = std::addressof(env),
            .depth = env.depth + 1
        };

        auto t1 = newEnv.newType(Type::Variable{ .depth = newEnv.depth });
        auto t2 = newEnv.newType(Type::Variable{ .depth = newEnv.depth });
        unify(rho, env.newType(Type::Function{ .paramType = t1, .returnType = t2 }));

        // 型環境にxを登録してeを評価
        newEnv.map.insert({ this->x, t1 });
        this->e->M(newEnv, t2);
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
    /// <param name="env">型環境</param>
    /// <returns>評価結果の型</returns>
    RefType J(TypeEnvironment& env) override {
        auto tau1 = this->e1->J(env);
        auto tau2 = this->e2->J(env);
        auto t = env.newType(Type::Variable{ .depth = env.depth });

        // 制約付きの場合は適用可能かの検査等を行うが未実装
        unify(tau1, env.newType(Type::Function{ .paramType = tau2, .returnType = t }));

        return t;
    }

    /// <summary>
    /// Algorithm Mの適用
    /// </summary>
    /// <param name="env">型環境</param>
    /// <param name="rho">式が推測される型</param>
    void M(TypeEnvironment& env, RefType rho) override {
        auto t = env.newType(Type::Variable{ .depth = env.depth });

        this->e1->M(env, env.newType(Type::Function{ .paramType = t, .returnType = rho }));
        this->e2->M(env, t);
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
    /// 束縛する式
    /// </summary>
    std::shared_ptr<Expression> e1;
    /// <summary>
    /// xを利用する式
    /// </summary>
    std::shared_ptr<Expression> e2;

    Let(std::string_view x, std::shared_ptr<Expression> e1, std::shared_ptr<Expression> e2) : x(x), e1(e1), e2(e2) {}
    ~Let() override {}

    /// <summary>
    /// Algorithm Jの適用
    /// </summary>
    /// <param name="env">型環境</param>
    /// <returns>評価結果の型</returns>
    RefType J(TypeEnvironment& env) override {
        auto tau1 = this->e1->J(env);
        // xが定義済みであっても型環境の改装を無視して上書きする
        // グローバルな型環境の場合は異常にする等があるかもしれない
        env.map.insert_or_assign(this->x, env.generalize(tau1));

        return this->e2->J(env);
    }

    /// <summary>
    /// Algorithm Mの適用
    /// </summary>
    /// <param name="env">型環境</param>
    /// <param name="rho">式が推測される型</param>
    void M(TypeEnvironment& env, RefType rho) override {
        auto t = env.newType(Type::Variable{ .depth = env.depth });

        this->e1->M(env, t);

        // xが定義済みであっても型環境の改装を無視して上書きする
        // グローバルな型環境の場合は異常にする等があるかもしれない
        env.map.insert_or_assign(this->x, env.generalize(t));
        this->e2->M(env, rho);
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
    /// 束縛する式
    /// </summary>
    std::shared_ptr<Expression> e1;
    /// <summary>
    /// xを利用する式
    /// </summary>
    std::shared_ptr<Expression> e2;

    Letrec(std::string_view x, std::shared_ptr<Expression> e1, std::shared_ptr<Expression> e2) : x(x), e1(e1), e2(e2) {}
    ~Letrec() override {}

    /// <summary>
    /// Algorithm Jの適用
    /// </summary>
    /// <param name="env">型環境</param>
    /// <returns>評価結果の型</returns>
    RefType J(TypeEnvironment& env) override {
        auto t = env.newType(Type::Variable{ .depth = env.depth });
        // xが定義済みであっても型環境の改装を無視して上書きする
        // グローバルな型環境の場合は異常にする等があるかもしれない
        env.map.insert_or_assign(this->x, t);

        auto tau1 = this->e1->J(env);
        unify(tau1, t);

        env.map.insert_or_assign(this->x, env.generalize(tau1));

        return this->e2->J(env);
    }

    /// <summary>
    /// Algorithm Mの適用
    /// </summary>
    /// <param name="env">型環境</param>
    /// <param name="rho">式が推測される型</param>
    void M(TypeEnvironment& env, RefType rho) override {
        auto t1 = env.newType(Type::Variable{ .depth = env.depth });
        auto t2 = env.newType(Type::Variable{ .depth = env.depth });
        // xが定義済みであっても型環境の改装を無視して上書きする
        // グローバルな型環境の場合は異常にする等があるかもしれない
        env.map.insert_or_assign(this->x, t1);

        this->e1->M(env, t2);
        unify(t1, t2);

        env.map.insert_or_assign(this->x, env.generalize(t1));
        this->e2->M(env, rho);
    }
};

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
                std::holds_alternative<Type::Param>(x.paramType->kind);
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
            }
        }
        void operator()(const Type::Param& x) {
            // 雑に[a, z]の範囲で型変数を出力
            // zを超えたら全て「_」で出力
            std::size_t c = 'a' + x.index;
            this->o << '\'' << static_cast<char>(c <= 'z' ? c : '_');
        }
    };
    std::visit(fn{ .o = os }, type->kind);

    return os;
}

// 雑に型を短く書くための関数
RefType base(TypeEnvironment& env, const std::string& name) { return env.newType(Type::Base{ .name = name }); }
RefType var(TypeEnvironment& env) { return env.newType(Type::Variable{ .depth = env.depth + 1 }); }
RefType fun(TypeEnvironment& env, RefType paramType, RefType returnType) { return env.newType(Type::Function{ .paramType = paramType, .returnType = returnType }); }

// 雑に構文を短く書くための関数
std::shared_ptr<Expression> c(RefType type) { return std::shared_ptr<Expression>(new Constant(type)); }
std::shared_ptr<Expression> id(const std::string& name) { return std::shared_ptr<Expression>(new Identifier(name)); }
std::shared_ptr<Expression> lambda(const std::string& name, std::shared_ptr<Expression> expr) { return std::shared_ptr<Expression>(new Lambda(name, expr)); }
std::shared_ptr<Expression> apply(std::shared_ptr<Expression> expr1, std::shared_ptr<Expression> expr2) { return std::shared_ptr<Expression>(new Apply(expr1, expr2)); }
template <class Head, class... Tail>
std::shared_ptr<Expression> apply(Head&& head1, Head&& head2, Tail&&... tail) { return apply(std::shared_ptr<Expression>(new Apply(head1, head2)), std::forward<Tail>(tail)...); }
std::shared_ptr<Expression> let(const std::string& name, std::shared_ptr<Expression> expr1, std::shared_ptr<Expression> expr2) { return std::shared_ptr<Expression>(new Let(name, expr1, expr2)); }
std::shared_ptr<Expression> letrec(const std::string& name, std::shared_ptr<Expression> expr1, std::shared_ptr<Expression> expr2) { return std::shared_ptr<Expression>(new Letrec(name, expr1, expr2)); }

int main() {
    // 型環境
    auto env = TypeEnvironment();

    // 適当に数値型とBoolean型を定義しておく
    auto numberT = base(env, "number");
    auto booleanT = base(env, "boolean");

    // 型環境に式を定義
    auto ifvalT = var(env);
    env.map.insert({ "if", env.generalize(fun(env, booleanT, fun(env, ifvalT, fun(env, ifvalT, ifvalT)))) });
    env.map.insert({ "-", fun(env, numberT, fun(env, numberT, numberT)) });
    env.map.insert({ "+", fun(env, numberT, fun(env, numberT, numberT)) });
    env.map.insert({ "<", fun(env, numberT, fun(env, numberT, booleanT)) });
    // 雑に定数も登録
    env.map.insert({ "true", booleanT });
    env.map.insert({ "false", booleanT });

    // 定数のつもりの構文を宣言しておく
    auto _1 = c(numberT);
    auto _2 = c(numberT);

    for (auto& expr :
        {
            // n -> 1
            lambda("n", _1),
            // n -> n - 1
            lambda("n", apply(id("-"), id("n"), _1)),
            // let id = n -> n in id id id id id 1
            let("id", lambda("n", id("n")), apply(id("id"), id("id"), id("id"), id("id"), id("id"), _1)),
            // letrec fib = n -> if n < 2 then n else fib(n - 1) + fib(n - 2) in fib
            letrec("fib", lambda("n",
                apply(id("if"), apply(id("<"), id("n"), _2),
                    id("n"),
                    apply(id("+"), apply(id("fib"), apply(id("-"), id("n"), _1)), apply(id("fib"), apply(id("-"), id("n"), _2)))
                )),
                id("fib")
            )
        })
    {
        // 型環境を使いまわして型推論をすると実質的にlet束縛で式を連結したことになってしまうが
        // 今回はシャドウも型環境の上書き禁止もないため許容する
        std::cout << "Algorithm J: " << expr->J(env) << std::endl;
        auto t = env.newType(Type::Variable{ .depth = env.depth - 1 });
        expr->M(env, t);
        std::cout << "Algorithm M: " << t << std::endl;
    }
}