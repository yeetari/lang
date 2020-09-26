#include <LLVMGen.hh>

#include <ir/BasicBlock.hh>
#include <ir/Constant.hh>
#include <ir/Function.hh>
#include <ir/Instructions.hh>
#include <ir/Program.hh>

#include <llvm/IR/IRBuilder.h>

#include <cassert>
#include <unordered_map>
#include <utility>

namespace {

class LLVMGen {
    const Program *const m_program;
    llvm::LLVMContext *const m_llvm_context;

    std::unique_ptr<llvm::Module> m_llvm_module;
    llvm::Function *m_llvm_function{nullptr};
    llvm::BasicBlock *m_llvm_block{nullptr};
    llvm::IRBuilder<> m_llvm_builder;
    std::unordered_map<const Value *, llvm::Value *> m_value_map;

public:
    LLVMGen(const Program *program, llvm::LLVMContext *llvm_context);

    llvm::Type *llvm_type(const Type *type);
    llvm::Value *llvm_value(const Value *value);

    llvm::Value *gen_load(const LoadInst *load);
    void gen_store(const StoreInst *store);
    void gen_ret(const RetInst *ret);

    llvm::Value *gen_constant(const Constant *constant);
    llvm::Value *gen_instruction(const Instruction *instruction);
    llvm::Value *gen_value(const Value *value);

    void gen_block(const BasicBlock *block);
    void gen_function(const Function *function);

    std::unique_ptr<llvm::Module> module() { return std::move(m_llvm_module); }
};

LLVMGen::LLVMGen(const Program *program, llvm::LLVMContext *llvm_context)
    : m_program(program), m_llvm_context(llvm_context), m_llvm_builder(*llvm_context) {
    m_llvm_module = std::make_unique<llvm::Module>("main", *llvm_context);
}

llvm::Type *LLVMGen::llvm_type(const Type *type) {
    switch (type->kind()) {
    case TypeKind::Invalid:
        assert(false);
    case TypeKind::Int:
        return llvm::Type::getIntNTy(*m_llvm_context, type->as<IntType>()->bit_width());
    case TypeKind::Pointer:
        return llvm::PointerType::get(llvm_type(type->as<PointerType>()->pointee_type()), 0);
    }
}

llvm::Value *LLVMGen::llvm_value(const Value *value) {
    if (!m_value_map.contains(value)) {
        m_value_map.emplace(value, gen_value(value));
    }
    return m_value_map.at(value);
}

llvm::Value *LLVMGen::gen_load(const LoadInst *load) {
    return m_llvm_builder.CreateLoad(llvm_value(load->ptr()));
}

void LLVMGen::gen_store(const StoreInst *store) {
    m_llvm_builder.CreateStore(llvm_value(store->val()), llvm_value(store->ptr()));
}

void LLVMGen::gen_ret(const RetInst *ret) {
    m_llvm_builder.CreateRet(llvm_value(ret->val()));
}

llvm::Value *LLVMGen::gen_constant(const Constant *constant) {
    return llvm::ConstantInt::get(llvm_type(constant->type()), constant->value());
}

llvm::Value *LLVMGen::gen_instruction(const Instruction *instruction) {
    switch (instruction->inst_kind()) {
    case InstKind::Load:
        return gen_load(instruction->as<LoadInst>());
    case InstKind::Store:
        gen_store(instruction->as<StoreInst>());
        return nullptr;
    case InstKind::Ret:
        gen_ret(instruction->as<RetInst>());
        return nullptr;
    default:
        assert(false);
    }
}

llvm::Value *LLVMGen::gen_value(const Value *value) {
    switch (value->kind()) {
    case ValueKind::Argument:
    case ValueKind::BasicBlock:
    case ValueKind::LocalVar:
        assert(false);
    case ValueKind::Constant:
        return gen_constant(value->as<Constant>());
    case ValueKind::Instruction:
        return gen_instruction(value->as<Instruction>());
    }
}

void LLVMGen::gen_block(const BasicBlock *block) {
    auto *new_block = llvm::BasicBlock::Create(*m_llvm_context, "", m_llvm_function);;
    if (m_llvm_block->empty() || !m_llvm_block->back().isTerminator()) {
        m_llvm_builder.CreateBr(new_block);
    }
    m_llvm_builder.SetInsertPoint(m_llvm_block = new_block);
    for (const auto *inst : *block) {
        llvm_value(inst);
    }
}

void LLVMGen::gen_function(const Function *function) {
    auto *function_type = llvm::FunctionType::get(llvm::Type::getInt32Ty(*m_llvm_context), false);
    m_llvm_function =
        llvm::Function::Create(function_type, llvm::Function::ExternalLinkage, function->name(), *m_llvm_module);
    m_llvm_block = llvm::BasicBlock::Create(*m_llvm_context, "vars", m_llvm_function);
    m_llvm_builder.SetInsertPoint(m_llvm_block);
    for (const auto *var : function->vars()) {
        auto *alloca = m_llvm_builder.CreateAlloca(llvm_type(var->var_type()));
        m_value_map.emplace(var, alloca);
    }
    for (const auto *block : *function) {
        gen_block(block);
    }
}

} // namespace

std::unique_ptr<llvm::Module> gen_llvm(const Program *program, llvm::LLVMContext *llvm_context) {
    LLVMGen gen(program, llvm_context);
    for (const auto *function : *program) {
        gen.gen_function(function);
    }
    return gen.module();
}
