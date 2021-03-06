#include <ir/Value.hh>

#include <ir/Types.hh>
#include <support/Assert.hh>

#include <algorithm>
#include <utility>

namespace ir {

Value::~Value() {
    replace_all_uses_with(nullptr);
}

void Value::add_user(Value *user) {
    m_users.push_back(user);
}

void Value::remove_user(Value *user) {
    auto it = std::find(m_users.begin(), m_users.end(), user);
    ASSERT(it != m_users.end());
    m_users.erase(it);
}

void Value::replace_all_uses_with(Value *repl) {
    if (repl == this) {
        return;
    }
    // TODO: Avoid copying, increment iterator properly.
    for (auto *user : std::vector(m_users)) {
        user->replace_uses_of_with(this, repl);
    }
    // Make sure that all users have removed themselves as users of us.
    ASSERT(m_users.empty());
}

void Value::replace_uses_of_with(Value *, Value *) {
    // Should be implemented by subclass.
    ASSERT_NOT_REACHED();
}

bool Value::has_type() const {
    ASSERT(m_type != nullptr);
    return !m_type->is<InvalidType>();
}

void Value::set_type(const Type *type) {
    m_type = type;
}

bool Value::has_name() const {
    return !m_name.empty();
}

void Value::set_name(std::string name) {
    m_name = std::move(name);
}

} // namespace ir
