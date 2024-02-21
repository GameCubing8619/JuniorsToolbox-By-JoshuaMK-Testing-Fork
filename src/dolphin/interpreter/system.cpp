#include "dolphin/interpreter/system.hpp"
#include "dolphin/interpreter/instructions/forms.hpp"
#include "dolphin/process.hpp"

#include "core/application.hpp"
#include "core/log.hpp"

using namespace Toolbox::Dolphin;

namespace Toolbox::Interpreter {

    void SystemDolphin::evaluateFunction() {
        while (m_evaluating.load()) {
            evaluateInstruction();
        }
    }

    void SystemDolphin::evaluateInstruction() {
        DolphinCommunicator &communicator = MainApplication::instance().getDolphinCommunicator();
        if (!communicator.manager().isHooked()) {
            m_evaluating.store(false);
            return;
        }

        u32 inst      = communicator.read<u32>(static_cast<u32>(m_system_proc.m_pc)).value();
        Opcode opcode = FORM_OPCD(inst);

        Register::PC next_instruction = m_system_proc.m_pc + 4;

        TOOLBOX_DEBUG_LOG_V("[Interpreter] PC: 0x{:08X}", m_system_proc.m_pc);

        switch (opcode) {
        case Opcode::OP_TWI:
            m_fixed_proc.twi(FORM_TO(inst), FORM_RA(inst), FORM_SI(inst));
            break;
        case Opcode::OP_PAIRED_SINGLE:
            evaluatePairedSingleSubOp(inst);
            break;
        case Opcode::OP_MULLI:
            m_fixed_proc.mulli(FORM_TO(inst), FORM_RA(inst), FORM_SI(inst));
            break;
        case Opcode::OP_SUBFIC:
            m_fixed_proc.subfic(FORM_TO(inst), FORM_RA(inst), FORM_SI(inst));
            break;
        case Opcode::OP_CMPLI:
            m_fixed_proc.cmpli(FORM_CRFD(inst), FORM_L(inst), FORM_RA(inst), FORM_UI(inst),
                               m_branch_proc.m_cr);
            break;
        case Opcode::OP_CMPL:
            m_fixed_proc.cmpli(FORM_CRFD(inst), FORM_L(inst), FORM_RA(inst), FORM_RB(inst),
                               m_branch_proc.m_cr);
            break;
        case Opcode::OP_ADDIC:
        case Opcode::OP_ADDIC_RC:
            m_fixed_proc.addic(FORM_RS(inst), FORM_RA(inst), FORM_SI(inst), FORM_RC(inst),
                               m_branch_proc.m_cr);
            break;
        case Opcode::OP_ADDI:
            m_fixed_proc.addi(FORM_RS(inst), FORM_RA(inst), FORM_SI(inst));
            break;
        case Opcode::OP_ADDIS:
            m_fixed_proc.addis(FORM_RS(inst), FORM_RA(inst), FORM_SI(inst));
            break;
        case Opcode::OP_BC:
            next_instruction -= 4;
            m_branch_proc.bc(FORM_BD(inst), FORM_BO(inst), FORM_BI(inst), FORM_AA(inst),
                             FORM_LK(inst), next_instruction);
            break;
        case Opcode::OP_SC:
            m_system_proc.sc(FORM_LEV(inst));
            break;
        case Opcode::OP_B:
            next_instruction -= 4;
            m_branch_proc.b(FORM_LI(inst), FORM_AA(inst), FORM_LK(inst), next_instruction);
            break;
        case Opcode::OP_CONTROL_FLOW:
            next_instruction = evaluateControlFlowSubOp(inst);
            break;
        case Opcode::OP_RLWIMI:
            m_fixed_proc.rlwimi(FORM_RA(inst), FORM_RS(inst), FORM_SH(inst), FORM_MB(inst),
                                FORM_ME(inst), FORM_RC(inst), m_branch_proc.m_cr);
            break;
        case Opcode::OP_RLWINM:
            m_fixed_proc.rlwinm(FORM_RA(inst), FORM_RS(inst), FORM_SH(inst), FORM_MB(inst),
                                FORM_ME(inst), FORM_RC(inst), m_branch_proc.m_cr);
            break;
        case Opcode::OP_RLWNM:
            m_fixed_proc.rlwnm(FORM_RA(inst), FORM_RS(inst), FORM_RB(inst), FORM_MB(inst),
                               FORM_ME(inst), FORM_RC(inst), m_branch_proc.m_cr);
            break;
        case Opcode::OP_ORI:
            m_fixed_proc.ori(FORM_RA(inst), FORM_RS(inst), FORM_UI(inst));
            break;
        case Opcode::OP_ORIS:
            m_fixed_proc.oris(FORM_RA(inst), FORM_RS(inst), FORM_UI(inst));
            break;
        case Opcode::OP_XORI:
            m_fixed_proc.xori(FORM_RA(inst), FORM_RS(inst), FORM_UI(inst));
            break;
        case Opcode::OP_XORIS:
            m_fixed_proc.xoris(FORM_RA(inst), FORM_RS(inst), FORM_UI(inst));
            break;
        case Opcode::OP_ANDI:
            m_fixed_proc.andi(FORM_RA(inst), FORM_RS(inst), FORM_UI(inst), m_branch_proc.m_cr);
            break;
        case Opcode::OP_ANDIS:
            m_fixed_proc.andis(FORM_RA(inst), FORM_RS(inst), FORM_UI(inst), m_branch_proc.m_cr);
            break;
        case Opcode::OP_FIXED:
            next_instruction = evaluateFixedSubOp(inst);
            break;
        case Opcode::OP_LWZ:
            m_fixed_proc.lwz(FORM_RS(inst), FORM_D(inst), FORM_RA(inst), m_storage);
            break;
        case Opcode::OP_LWZU:
            m_fixed_proc.lwzu(FORM_RS(inst), FORM_D(inst), FORM_RA(inst), m_storage);
            break;
        case Opcode::OP_LBZ:
            m_fixed_proc.lbz(FORM_RS(inst), FORM_D(inst), FORM_RA(inst), m_storage);
            break;
        case Opcode::OP_LBZU:
            m_fixed_proc.lbzu(FORM_RS(inst), FORM_D(inst), FORM_RA(inst), m_storage);
            break;
        case Opcode::OP_STW:
            m_fixed_proc.stw(FORM_RS(inst), FORM_D(inst), FORM_RA(inst), m_storage);
            break;
        case Opcode::OP_STWU:
            m_fixed_proc.stwu(FORM_RS(inst), FORM_D(inst), FORM_RA(inst), m_storage);
            break;
        case Opcode::OP_STB:
            m_fixed_proc.stb(FORM_RS(inst), FORM_D(inst), FORM_RA(inst), m_storage);
            break;
        case Opcode::OP_STBU:
            m_fixed_proc.stbu(FORM_RS(inst), FORM_D(inst), FORM_RA(inst), m_storage);
            break;
        case Opcode::OP_LHZ:
            m_fixed_proc.lhz(FORM_RS(inst), FORM_D(inst), FORM_RA(inst), m_storage);
            break;
        case Opcode::OP_LHZU:
            m_fixed_proc.lhzu(FORM_RS(inst), FORM_D(inst), FORM_RA(inst), m_storage);
            break;
        case Opcode::OP_LHA:
            m_fixed_proc.lha(FORM_RS(inst), FORM_D(inst), FORM_RA(inst), m_storage);
            break;
        case Opcode::OP_LHAU:
            m_fixed_proc.lhau(FORM_RS(inst), FORM_D(inst), FORM_RA(inst), m_storage);
            break;
        case Opcode::OP_STH:
            m_fixed_proc.sth(FORM_RS(inst), FORM_D(inst), FORM_RA(inst), m_storage);
            break;
        case Opcode::OP_STHU:
            m_fixed_proc.sthu(FORM_RS(inst), FORM_D(inst), FORM_RA(inst), m_storage);
            break;
        case Opcode::OP_LMW:
            m_fixed_proc.lmw(FORM_RS(inst), FORM_D(inst), FORM_RA(inst), m_storage);
            break;
        case Opcode::OP_STMW:
            m_fixed_proc.stmw(FORM_RS(inst), FORM_D(inst), FORM_RA(inst), m_storage);
            break;
        case Opcode::OP_LFS:
            m_float_proc.lfs(FORM_FS(inst), FORM_D(inst), FORM_RA(inst), m_storage);
            break;
        case Opcode::OP_LFSU:
            m_float_proc.lfsu(FORM_FS(inst), FORM_D(inst), FORM_RA(inst), m_storage);
            break;
        case Opcode::OP_LFD:
            m_float_proc.lfd(FORM_FS(inst), FORM_D(inst), FORM_RA(inst), m_storage);
            break;
        case Opcode::OP_LFDU:
            m_float_proc.lfdu(FORM_FS(inst), FORM_D(inst), FORM_RA(inst), m_storage);
            break;
        case Opcode::OP_STFS:
            m_float_proc.stfs(FORM_FS(inst), FORM_D(inst), FORM_RA(inst), m_storage);
            break;
        case Opcode::OP_STFSU:
            m_float_proc.stfsu(FORM_FS(inst), FORM_D(inst), FORM_RA(inst), m_storage);
            break;
        case Opcode::OP_STFD:
            m_float_proc.stfd(FORM_FS(inst), FORM_D(inst), FORM_RA(inst), m_storage);
            break;
        case Opcode::OP_STFDU:
            m_float_proc.stfdu(FORM_FS(inst), FORM_D(inst), FORM_RA(inst), m_storage);
            break;
        case Opcode::OP_FS_MATH:
            next_instruction = evaluateFloatSingleSubOp(inst);
            break;
        case Opcode::OP_FLOAT:
            next_instruction = evaluateFloatSubOp(inst);
            break;
        default:
            internalInvalidCB(PROC_INVALID_MSG(SystemDolphin, unknown,
                                               "Attempted to evaluate unknown instruction!"));
            break;
        }
        m_system_proc.m_pc = next_instruction;
    }

    u32 SystemDolphin::evaluatePairedSingleSubOp(u32 inst) {
        TableSubOpcode4 sub_op = (TableSubOpcode4)FORM_XO_5(inst);
        switch (sub_op) {
        default:
            internalInvalidCB(PROC_INVALID_MSG(SystemDolphin, unknown,
                                               "Attempted to evaluate unknown instruction!"));
            break;
        }
        return m_system_proc.m_pc + 4;
    }

    u32 SystemDolphin::evaluateControlFlowSubOp(u32 inst) {
        Register::PC next_instruction = m_system_proc.m_pc + 4;

        TableSubOpcode19 sub_op = (TableSubOpcode19)FORM_XO_9(inst);
        switch (sub_op) {
        case TableSubOpcode19::MCRF:
            m_branch_proc.mcrf(FORM_CRBD(inst), FORM_CRBA(inst));
            break;
        case TableSubOpcode19::BCLR:
            next_instruction -= 4;
            m_branch_proc.bclr(FORM_BO(inst), FORM_BI(inst), FORM_LK(inst), next_instruction);
            break;
        case TableSubOpcode19::CRNOR:
            m_branch_proc.crnor(FORM_CRBD(inst), FORM_CRBA(inst), FORM_CRBB(inst));
            break;
        case TableSubOpcode19::RFI:
            m_system_proc.rfi();
            break;
        case TableSubOpcode19::CRANDC:
            m_branch_proc.crandc(FORM_CRBD(inst), FORM_CRBA(inst), FORM_CRBB(inst));
            break;
        case TableSubOpcode19::ISYNC:
            m_system_proc.sync((SyncType)0);
            break;
        case TableSubOpcode19::CRXOR:
            m_branch_proc.crxor(FORM_CRBD(inst), FORM_CRBA(inst), FORM_CRBB(inst));
            break;
        case TableSubOpcode19::CRNAND:
            m_branch_proc.crnand(FORM_CRBD(inst), FORM_CRBA(inst), FORM_CRBB(inst));
            break;
        case TableSubOpcode19::CRAND:
            m_branch_proc.crand(FORM_CRBD(inst), FORM_CRBA(inst), FORM_CRBB(inst));
            break;
        case TableSubOpcode19::CREQV:
            m_branch_proc.creqv(FORM_CRBD(inst), FORM_CRBA(inst), FORM_CRBB(inst));
            break;
        case TableSubOpcode19::CRORC:
            m_branch_proc.crorc(FORM_CRBD(inst), FORM_CRBA(inst), FORM_CRBB(inst));
            break;
        case TableSubOpcode19::CROR:
            m_branch_proc.cror(FORM_CRBD(inst), FORM_CRBA(inst), FORM_CRBB(inst));
            break;
        case TableSubOpcode19::BCCTR:
            m_branch_proc.bcctr(FORM_BO(inst), FORM_BI(inst), FORM_LK(inst), m_system_proc.m_pc);
        default:
            internalInvalidCB(PROC_INVALID_MSG(SystemDolphin, unknown,
                                               "Attempted to evaluate unknown instruction!"));
            break;
        }

        return next_instruction;
    }

    u32 SystemDolphin::evaluateFixedSubOp(u32 inst) {
        TableSubOpcode31 sub_op = (TableSubOpcode31)FORM_XO_9(inst);
        switch (sub_op) {
        case TableSubOpcode31::ADD:
        case TableSubOpcode31::ADDO:
            m_fixed_proc.add(FORM_RS(inst), FORM_RA(inst), FORM_RB(inst), FORM_OE(inst),
                             FORM_RC(inst), m_branch_proc.m_cr);
            break;
        case TableSubOpcode31::ADDC:
        case TableSubOpcode31::ADDCO:
            m_fixed_proc.addc(FORM_RS(inst), FORM_RA(inst), FORM_RB(inst), FORM_OE(inst),
                              FORM_RC(inst), m_branch_proc.m_cr);
            break;
        case TableSubOpcode31::ADDE:
        case TableSubOpcode31::ADDEO:
            m_fixed_proc.adde(FORM_RS(inst), FORM_RA(inst), FORM_RB(inst), FORM_OE(inst),
                              FORM_RC(inst), m_branch_proc.m_cr);
            break;
        case TableSubOpcode31::ADDME:
        case TableSubOpcode31::ADDMEO:
            m_fixed_proc.addme(FORM_RS(inst), FORM_RA(inst), FORM_OE(inst), FORM_RC(inst),
                               m_branch_proc.m_cr);
            break;
        case TableSubOpcode31::ADDZE:
        case TableSubOpcode31::ADDZEO:
            m_fixed_proc.addze(FORM_RS(inst), FORM_RA(inst), FORM_OE(inst), FORM_RC(inst),
                               m_branch_proc.m_cr);
            break;
        case TableSubOpcode31::DIVW:
        case TableSubOpcode31::DIVWO:
            m_fixed_proc.divw(FORM_RS(inst), FORM_RA(inst), FORM_RB(inst), FORM_OE(inst),
                              FORM_RC(inst), m_branch_proc.m_cr);
            break;
        case TableSubOpcode31::DIVWU:
        case TableSubOpcode31::DIVWUO:
            m_fixed_proc.divwu(FORM_RS(inst), FORM_RA(inst), FORM_RB(inst), FORM_OE(inst),
                               FORM_RC(inst), m_branch_proc.m_cr);
            break;
        case TableSubOpcode31::MULHW:
            m_fixed_proc.mullhw(FORM_RS(inst), FORM_RA(inst), FORM_RB(inst), FORM_RC(inst),
                                m_branch_proc.m_cr);
            break;
        case TableSubOpcode31::MULHWU:
            m_fixed_proc.mullhwu(FORM_RS(inst), FORM_RA(inst), FORM_RB(inst), FORM_RC(inst),
                                 m_branch_proc.m_cr);
            break;
        case TableSubOpcode31::MULLW:
        case TableSubOpcode31::MULLWO:
            m_fixed_proc.mullw(FORM_RS(inst), FORM_RA(inst), FORM_RB(inst), FORM_OE(inst),
                               FORM_RC(inst), m_branch_proc.m_cr);
            break;
        case TableSubOpcode31::NEG:
        case TableSubOpcode31::NEGO:
            break;
        case TableSubOpcode31::SUBF:
        case TableSubOpcode31::SUBFO:
            m_fixed_proc.subf(FORM_RS(inst), FORM_RA(inst), FORM_RB(inst), FORM_OE(inst),
                              FORM_RC(inst), m_branch_proc.m_cr);
            break;
        case TableSubOpcode31::SUBFC:
        case TableSubOpcode31::SUBFCO:
            m_fixed_proc.subfc(FORM_RS(inst), FORM_RA(inst), FORM_RB(inst), FORM_OE(inst),
                               FORM_RC(inst), m_branch_proc.m_cr);
            break;
        case TableSubOpcode31::SUBFE:
        case TableSubOpcode31::SUBFEO:
            m_fixed_proc.subfe(FORM_RS(inst), FORM_RA(inst), FORM_RB(inst), FORM_OE(inst),
                               FORM_RC(inst), m_branch_proc.m_cr);
            break;
        case TableSubOpcode31::SUBFME:
        case TableSubOpcode31::SUBFMEO:
            m_fixed_proc.subfme(FORM_RS(inst), FORM_RA(inst), FORM_OE(inst), FORM_RC(inst),
                                m_branch_proc.m_cr);
            break;
        case TableSubOpcode31::SUBFZE:
        case TableSubOpcode31::SUBFZEO:
            m_fixed_proc.subfze(FORM_RS(inst), FORM_RA(inst), FORM_OE(inst), FORM_RC(inst),
                                m_branch_proc.m_cr);
            break;
        case TableSubOpcode31::AND:
            m_fixed_proc.and_(FORM_RA(inst), FORM_RS(inst), FORM_RB(inst), FORM_RC(inst),
                              m_branch_proc.m_cr);
            break;
        case TableSubOpcode31::ANDC:
            m_fixed_proc.andc(FORM_RA(inst), FORM_RS(inst), FORM_RB(inst), FORM_RC(inst),
                              m_branch_proc.m_cr);
            break;
        case TableSubOpcode31::OR:
            m_fixed_proc.or_(FORM_RA(inst), FORM_RS(inst), FORM_RB(inst), FORM_RC(inst),
                             m_branch_proc.m_cr);
            break;
        case TableSubOpcode31::NOR:
            m_fixed_proc.nor_(FORM_RA(inst), FORM_RS(inst), FORM_RB(inst), FORM_RC(inst),
                              m_branch_proc.m_cr);
            break;
        case TableSubOpcode31::XOR:
            m_fixed_proc.xor_(FORM_RA(inst), FORM_RS(inst), FORM_RB(inst), FORM_RC(inst),
                              m_branch_proc.m_cr);
            break;
        case TableSubOpcode31::ORC:
            m_fixed_proc.orc(FORM_RA(inst), FORM_RS(inst), FORM_RB(inst), FORM_RC(inst),
                             m_branch_proc.m_cr);
            break;
        case TableSubOpcode31::NAND:
            m_fixed_proc.nand_(FORM_RA(inst), FORM_RS(inst), FORM_RB(inst), FORM_RC(inst),
                               m_branch_proc.m_cr);
            break;
        case TableSubOpcode31::EQV:
            m_fixed_proc.eqv_(FORM_RA(inst), FORM_RS(inst), FORM_RB(inst), FORM_RC(inst),
                              m_branch_proc.m_cr);
            break;
        case TableSubOpcode31::CMP:
            m_fixed_proc.cmp(FORM_CRFD(inst), FORM_L(inst), FORM_RA(inst), FORM_RB(inst),
                             m_branch_proc.m_cr);
            break;
        case TableSubOpcode31::CMPL:
            m_fixed_proc.cmpl(FORM_CRFD(inst), FORM_L(inst), FORM_RA(inst), FORM_RB(inst),
                              m_branch_proc.m_cr);
            break;
        case TableSubOpcode31::CNTLZW:
            m_fixed_proc.cntlzw(FORM_RA(inst), FORM_RB(inst), FORM_RC(inst), m_branch_proc.m_cr);
            break;
        case TableSubOpcode31::EXTSH:
            m_fixed_proc.extsh(FORM_RA(inst), FORM_RB(inst), FORM_RC(inst), m_branch_proc.m_cr);
            break;
        case TableSubOpcode31::EXTSB:
            m_fixed_proc.extsb(FORM_RA(inst), FORM_RB(inst), FORM_RC(inst), m_branch_proc.m_cr);
            break;
        case TableSubOpcode31::SRW:
            m_fixed_proc.srw(FORM_RA(inst), FORM_RS(inst), FORM_RB(inst), FORM_RC(inst),
                             m_branch_proc.m_cr);
            break;
        case TableSubOpcode31::SRAW:
            m_fixed_proc.sraw(FORM_RA(inst), FORM_RS(inst), FORM_RB(inst), FORM_RC(inst),
                              m_branch_proc.m_cr);
            break;
        case TableSubOpcode31::SRAWI:
            m_fixed_proc.srawi(FORM_RA(inst), FORM_RS(inst), FORM_SH(inst), FORM_RC(inst),
                               m_branch_proc.m_cr);
            break;
        case TableSubOpcode31::SLW:
            m_fixed_proc.slw(FORM_RA(inst), FORM_RS(inst), FORM_RB(inst), FORM_RC(inst),
                             m_branch_proc.m_cr);
            break;
        case TableSubOpcode31::DCBST:
            m_system_proc.dcbst(FORM_RA(inst), FORM_RB(inst), m_storage);
            break;
        case TableSubOpcode31::DCBF:
            m_system_proc.dcbf(FORM_RA(inst), FORM_RB(inst), FORM_L(inst), m_storage);
            break;
        case TableSubOpcode31::DCBTST:
            m_system_proc.dcbtst(FORM_RA(inst), FORM_RB(inst), m_storage);
            break;
        case TableSubOpcode31::DCBT:
            m_system_proc.dcbst(FORM_RA(inst), FORM_RB(inst), m_storage);
            break;
        case TableSubOpcode31::DCBI:
            m_system_proc.dcbi(FORM_RA(inst), FORM_RB(inst), m_storage);
            break;
        case TableSubOpcode31::DCBA:
        case TableSubOpcode31::DCBZ:
            m_system_proc.dcbz(FORM_RA(inst), FORM_RB(inst), m_storage);
            break;
        case TableSubOpcode31::LWZ:
            m_fixed_proc.lwz(FORM_RD(inst), FORM_D(inst), FORM_RA(inst), m_storage);
            break;
        case TableSubOpcode31::LWZU:
            m_fixed_proc.lwzu(FORM_RD(inst), FORM_D(inst), FORM_RA(inst), m_storage);
            break;
        case TableSubOpcode31::LHZ:
            m_fixed_proc.lhz(FORM_RD(inst), FORM_D(inst), FORM_RA(inst), m_storage);
            break;
        case TableSubOpcode31::LHZU:
            m_fixed_proc.lhzu(FORM_RD(inst), FORM_D(inst), FORM_RA(inst), m_storage);
            break;
        case TableSubOpcode31::LHA:
            m_fixed_proc.lha(FORM_RD(inst), FORM_D(inst), FORM_RA(inst), m_storage);
            break;
        case TableSubOpcode31::LHAU:
            m_fixed_proc.lhau(FORM_RD(inst), FORM_D(inst), FORM_RA(inst), m_storage);
            break;
        case TableSubOpcode31::LBZ:
            m_fixed_proc.lbz(FORM_RD(inst), FORM_D(inst), FORM_RA(inst), m_storage);
            break;
        case TableSubOpcode31::LBZU:
            m_fixed_proc.lbzu(FORM_RD(inst), FORM_D(inst), FORM_RA(inst), m_storage);
            break;
        case TableSubOpcode31::LWBRX:
            m_fixed_proc.lwbrx(FORM_RD(inst), FORM_RA(inst), FORM_RB(inst), m_storage);
            break;
        case TableSubOpcode31::LHBRX:
            m_fixed_proc.lhbrx(FORM_RD(inst), FORM_RA(inst), FORM_RB(inst), m_storage);
            break;
        case TableSubOpcode31::STWCXD:
            break;
        case TableSubOpcode31::LWARX:
            internalInvalidCB(PROC_INVALID_MSG(SystemDolphin, lwarx,
                                               "Attempted to evaluate unknown instruction!"));
            break;
        case TableSubOpcode31::LSWX:
            m_fixed_proc.lswx(FORM_RD(inst), FORM_RA(inst), FORM_RB(inst), m_storage);
            break;
        case TableSubOpcode31::LSWI:
            m_fixed_proc.lswi(FORM_RD(inst), FORM_RA(inst), FORM_NB(inst), m_storage);
            break;
        case TableSubOpcode31::STW:
            m_fixed_proc.stw(FORM_RS(inst), FORM_D(inst), FORM_RA(inst), m_storage);
            break;
        case TableSubOpcode31::STWU:
            m_fixed_proc.stwu(FORM_RS(inst), FORM_D(inst), FORM_RA(inst), m_storage);
            break;
        case TableSubOpcode31::STH:
            m_fixed_proc.sth(FORM_RS(inst), FORM_D(inst), FORM_RA(inst), m_storage);
            break;
        case TableSubOpcode31::STHU:
            m_fixed_proc.sthu(FORM_RS(inst), FORM_D(inst), FORM_RA(inst), m_storage);
            break;
        case TableSubOpcode31::STB:
            m_fixed_proc.stb(FORM_RS(inst), FORM_D(inst), FORM_RA(inst), m_storage);
            break;
        case TableSubOpcode31::STBU:
            m_fixed_proc.stbu(FORM_RS(inst), FORM_D(inst), FORM_RA(inst), m_storage);
            break;
        case TableSubOpcode31::STWBRX:
            m_fixed_proc.stwbrx(FORM_RS(inst), FORM_RA(inst), FORM_RB(inst), m_storage);
            break;
        case TableSubOpcode31::STHBRX:
            m_fixed_proc.sthbrx(FORM_RS(inst), FORM_RA(inst), FORM_RB(inst), m_storage);
            break;
        case TableSubOpcode31::STSWX:
            m_fixed_proc.stswx(FORM_RS(inst), FORM_RA(inst), FORM_RB(inst), m_storage);
            break;
        case TableSubOpcode31::STSWI:
            m_fixed_proc.stswi(FORM_RS(inst), FORM_RA(inst), FORM_NB(inst), m_storage);
            break;
        case TableSubOpcode31::LFS:
            m_float_proc.lfs(FORM_FS(inst), FORM_D(inst), FORM_RA(inst), m_storage);
            break;
        case TableSubOpcode31::LFSU:
            m_float_proc.lfsu(FORM_FS(inst), FORM_D(inst), FORM_RA(inst), m_storage);
            break;
        case TableSubOpcode31::LFD:
            m_float_proc.lfd(FORM_FS(inst), FORM_D(inst), FORM_RA(inst), m_storage);
            break;
        case TableSubOpcode31::LFDU:
            m_float_proc.lfdu(FORM_FS(inst), FORM_D(inst), FORM_RA(inst), m_storage);
            break;
        case TableSubOpcode31::STFS:
            m_float_proc.stfs(FORM_FS(inst), FORM_D(inst), FORM_RA(inst), m_storage);
            break;
        case TableSubOpcode31::STFSU:
            m_float_proc.stfsu(FORM_FS(inst), FORM_D(inst), FORM_RA(inst), m_storage);
            break;
        case TableSubOpcode31::STFD:
            m_float_proc.stfd(FORM_FS(inst), FORM_D(inst), FORM_RA(inst), m_storage);
            break;
        case TableSubOpcode31::STFDU:
            m_float_proc.stfdu(FORM_FS(inst), FORM_D(inst), FORM_RA(inst), m_storage);
            break;
        case TableSubOpcode31::STFIWX:
            m_float_proc.stfiwx(FORM_FS(inst), FORM_RA(inst), FORM_RB(inst), m_storage);
            break;
        case TableSubOpcode31::MFCR:
            m_fixed_proc.mfcr(FORM_RS(inst), m_branch_proc.m_cr);
            break;
        case TableSubOpcode31::MFMSR:
            m_fixed_proc.mfmsr(FORM_RS(inst), m_system_proc.m_msr);
            break;
        case TableSubOpcode31::MTCRF:
            m_fixed_proc.mtcrf(FORM_CRM(inst), FORM_RS(inst), m_branch_proc.m_cr);
            break;
        case TableSubOpcode31::MTMSR:
            m_fixed_proc.mtmsr(FORM_RS(inst), m_system_proc.m_msr);
            break;
        case TableSubOpcode31::MTSR:
            internalInvalidCB(PROC_INVALID_MSG(SystemDolphin, mtsr,
                                               "Attempted to evaluate unknown instruction!"));
            break;
        case TableSubOpcode31::MTSRIN:
            internalInvalidCB(PROC_INVALID_MSG(SystemDolphin, mtsrin,
                                               "Attempted to evaluate unknown instruction!"));
            break;
        case TableSubOpcode31::MFSPR:
            m_fixed_proc.mfspr((Register::SPRType)FORM_SPR(inst), FORM_RD(inst), m_branch_proc.m_lr,
                               m_branch_proc.m_ctr);
            break;
        case TableSubOpcode31::MTSPR:
            m_fixed_proc.mtspr((Register::SPRType)FORM_SPR(inst), FORM_RS(inst), m_branch_proc.m_lr,
                               m_branch_proc.m_ctr);
            break;
        case TableSubOpcode31::MFTB:
            m_fixed_proc.mftb(FORM_RD(inst), FORM_TBR(inst), m_system_proc.m_tb);
            break;
        case TableSubOpcode31::MCRXR:
            m_fixed_proc.mcrxr(m_branch_proc.m_cr, FORM_CRFD(inst));
            break;
        case TableSubOpcode31::MFSR:
            internalInvalidCB(
                PROC_INVALID_MSG(SystemDolphin, mfsr "Attempted to evaluate unknown instruction!"));
            break;
        case TableSubOpcode31::MFSRIN:
            internalInvalidCB(PROC_INVALID_MSG(SystemDolphin, mfsrin,
                                               "Attempted to evaluate unknown instruction!"));
            break;
        case TableSubOpcode31::TW:
            m_fixed_proc.tw(FORM_TO(inst), FORM_RA(inst), FORM_RB(inst));
            break;
        case TableSubOpcode31::SYNC:
            m_system_proc.sync((SyncType)FORM_I(inst));
            break;
        case TableSubOpcode31::ICBI:
            m_system_proc.icbi(FORM_RA(inst), FORM_RB(inst), m_storage);
            break;
        case TableSubOpcode31::ECIWX:
            m_fixed_proc.eciwx(FORM_RD(inst), FORM_RA(inst), FORM_RB(inst), m_storage);
            break;
        case TableSubOpcode31::ECOWX:
            m_fixed_proc.ecowx(FORM_RS(inst), FORM_RA(inst), FORM_RB(inst), m_storage);
            break;
        case TableSubOpcode31::EIEIO:
            m_system_proc.eieio();
            break;
        case TableSubOpcode31::TLBIE:
            internalInvalidCB(PROC_INVALID_MSG(SystemDolphin, tlbie,
                                               "Attempted to evaluate unknown instruction!"));
            break;
        case TableSubOpcode31::TLBSYNC:
            internalInvalidCB(PROC_INVALID_MSG(SystemDolphin, tlbsync,
                                               "Attempted to evaluate unknown instruction!"));
            break;
        }
        return m_system_proc.m_pc + 4;
    }

    u32 SystemDolphin::evaluateFloatSingleSubOp(u32 inst) {
        TableSubOpcode59 sub_op = (TableSubOpcode59)FORM_XO_5(inst);
        switch (sub_op) {
        case TableSubOpcode59::FDIVS:
            m_float_proc.fdivs(FORM_FS(inst), FORM_FA(inst), FORM_FB(inst), FORM_RC(inst),
                               m_branch_proc.m_cr, m_system_proc.m_msr, m_system_proc.m_srr1);
            break;
        case TableSubOpcode59::FSUBS:
            m_float_proc.fsubs(FORM_FS(inst), FORM_FA(inst), FORM_FB(inst), FORM_RC(inst),
                               m_branch_proc.m_cr, m_system_proc.m_msr, m_system_proc.m_srr1);
            break;
        case TableSubOpcode59::FADDS:
            m_float_proc.fadds(FORM_FS(inst), FORM_FA(inst), FORM_FB(inst), FORM_RC(inst),
                               m_branch_proc.m_cr, m_system_proc.m_msr, m_system_proc.m_srr1);
            break;
        case TableSubOpcode59::FRES:
            m_float_proc.fres(FORM_FS(inst), FORM_FA(inst), FORM_RC(inst), m_branch_proc.m_cr,
                              m_system_proc.m_msr, m_system_proc.m_srr1);
            break;
        case TableSubOpcode59::FMULS:
            m_float_proc.fmuls(FORM_FS(inst), FORM_FA(inst), FORM_FB(inst), FORM_RC(inst),
                               m_branch_proc.m_cr, m_system_proc.m_msr, m_system_proc.m_srr1);
            break;
        case TableSubOpcode59::FMSUBS:
            m_float_proc.fmsubs(FORM_FS(inst), FORM_FA(inst), FORM_FC(inst), FORM_FB(inst),
                                FORM_RC(inst), m_branch_proc.m_cr, m_system_proc.m_msr,
                                m_system_proc.m_srr1);
            break;
        case TableSubOpcode59::FMADDS:
            m_float_proc.fmadds(FORM_FS(inst), FORM_FA(inst), FORM_FC(inst), FORM_FB(inst),
                                FORM_RC(inst), m_branch_proc.m_cr, m_system_proc.m_msr,
                                m_system_proc.m_srr1);
            break;
        case TableSubOpcode59::FNMSUBS:
            m_float_proc.fnmsubs(FORM_FS(inst), FORM_FA(inst), FORM_FC(inst), FORM_FB(inst),
                                 FORM_RC(inst), m_branch_proc.m_cr, m_system_proc.m_msr,
                                 m_system_proc.m_srr1);
            break;
        case TableSubOpcode59::FNMADDS:
            m_float_proc.fnmadds(FORM_FS(inst), FORM_FA(inst), FORM_FC(inst), FORM_FB(inst),
                                 FORM_RC(inst), m_branch_proc.m_cr, m_system_proc.m_msr,
                                 m_system_proc.m_srr1);
            break;
        default:
            internalInvalidCB(PROC_INVALID_MSG(SystemDolphin, unknown,
                                               "Attempted to evaluate unknown instruction!"));
            break;
        }
        return m_system_proc.m_pc + 4;
    }

    u32 SystemDolphin::evaluateFloatSubOp(u32 inst) {
        TableSubOpcode63 sub_op = (TableSubOpcode63)FORM_XO_5(inst);
        switch (sub_op) {
        case TableSubOpcode63::FCMPU:
            m_float_proc.fcmpu(FORM_CRFD(inst), FORM_FA(inst), FORM_FB(inst), m_branch_proc.m_cr,
                               m_system_proc.m_msr, m_system_proc.m_srr1);
            break;
        case TableSubOpcode63::FRSP:
            m_float_proc.frsp(FORM_FS(inst), FORM_FB(inst), FORM_RC(inst), m_branch_proc.m_cr,
                              m_system_proc.m_msr, m_system_proc.m_srr1);
            break;
        case TableSubOpcode63::FCTIW:
            m_float_proc.fctiw(FORM_FS(inst), FORM_FB(inst), FORM_RC(inst), m_branch_proc.m_cr,
                               m_system_proc.m_msr, m_system_proc.m_srr1);
            break;
        case TableSubOpcode63::FCTIWZ:
            m_float_proc.fctiwz(FORM_FS(inst), FORM_FB(inst), FORM_RC(inst), m_branch_proc.m_cr,
                                m_system_proc.m_msr, m_system_proc.m_srr1);
            break;
        case TableSubOpcode63::FDIV:
            m_float_proc.fdiv(FORM_FS(inst), FORM_FA(inst), FORM_FB(inst), FORM_RC(inst),
                              m_branch_proc.m_cr, m_system_proc.m_msr, m_system_proc.m_srr1);
            break;
        case TableSubOpcode63::FSUB:
            m_float_proc.fsub(FORM_FS(inst), FORM_FA(inst), FORM_FB(inst), FORM_RC(inst),
                              m_branch_proc.m_cr, m_system_proc.m_msr, m_system_proc.m_srr1);
            break;
        case TableSubOpcode63::FADD:
            m_float_proc.fadd(FORM_FS(inst), FORM_FA(inst), FORM_FB(inst), FORM_RC(inst),
                              m_branch_proc.m_cr, m_system_proc.m_msr, m_system_proc.m_srr1);
            break;
        case TableSubOpcode63::FRES:
            m_float_proc.fres(FORM_FS(inst), FORM_FB(inst), FORM_RC(inst), m_branch_proc.m_cr,
                              m_system_proc.m_msr, m_system_proc.m_srr1);
            break;
        case TableSubOpcode63::FMUL:
            m_float_proc.fmul(FORM_FS(inst), FORM_FA(inst), FORM_FB(inst), FORM_RC(inst),
                              m_branch_proc.m_cr, m_system_proc.m_msr, m_system_proc.m_srr1);
            break;
        case TableSubOpcode63::FMSUB:
            m_float_proc.fmsub(FORM_FS(inst), FORM_FA(inst), FORM_FC(inst), FORM_FB(inst),
                               FORM_RC(inst), m_branch_proc.m_cr, m_system_proc.m_msr,
                               m_system_proc.m_srr1);
            break;
        case TableSubOpcode63::FMADD:
            m_float_proc.fmadd(FORM_FS(inst), FORM_FA(inst), FORM_FC(inst), FORM_FB(inst),
                               FORM_RC(inst), m_branch_proc.m_cr, m_system_proc.m_msr,
                               m_system_proc.m_srr1);
            break;
        case TableSubOpcode63::FNMSUB:
            m_float_proc.fnmsub(FORM_FS(inst), FORM_FA(inst), FORM_FC(inst), FORM_FB(inst),
                                FORM_RC(inst), m_branch_proc.m_cr, m_system_proc.m_msr,
                                m_system_proc.m_srr1);
            break;
        case TableSubOpcode63::FNMADD:
            m_float_proc.fnmadd(FORM_FS(inst), FORM_FA(inst), FORM_FC(inst), FORM_FB(inst),
                                FORM_RC(inst), m_branch_proc.m_cr, m_system_proc.m_msr,
                                m_system_proc.m_srr1);
            break;
        case TableSubOpcode63::FCMPO:
            m_float_proc.fcmpo(FORM_CRFD(inst), FORM_FA(inst), FORM_FB(inst), m_branch_proc.m_cr,
                               m_system_proc.m_msr, m_system_proc.m_srr1);
            break;
        case TableSubOpcode63::MTFSB1:
            m_float_proc.mtfsb1(FORM_CRBD(inst), FORM_RC(inst), m_branch_proc.m_cr,
                                m_system_proc.m_msr, m_system_proc.m_srr1);
            break;
        case TableSubOpcode63::FNEG:
            m_float_proc.fneg(FORM_FS(inst), FORM_FB(inst), FORM_RC(inst), m_branch_proc.m_cr,
                              m_system_proc.m_msr, m_system_proc.m_srr1);
            break;
        case TableSubOpcode63::MCRFS:
            m_float_proc.mcrfs(FORM_CRBD(inst), FORM_CRBA(inst), m_branch_proc.m_cr,
                               m_system_proc.m_msr, m_system_proc.m_srr1);
            break;
        case TableSubOpcode63::MTFSB0:
            m_float_proc.mtfsb0(FORM_CRBD(inst), FORM_RC(inst), m_branch_proc.m_cr,
                                m_system_proc.m_msr, m_system_proc.m_srr1);
            break;
        case TableSubOpcode63::FMR:
            m_float_proc.fmr(FORM_CRFD(inst), FORM_FB(inst), FORM_RC(inst), m_branch_proc.m_cr,
                             m_system_proc.m_msr, m_system_proc.m_srr1);
            break;
        case TableSubOpcode63::MTFSFI:
            break;
        case TableSubOpcode63::FNABS:
            m_float_proc.fnabs(FORM_CRFD(inst), FORM_FB(inst), FORM_RC(inst), m_branch_proc.m_cr,
                               m_system_proc.m_msr, m_system_proc.m_srr1);
            break;
        case TableSubOpcode63::FABS:
            m_float_proc.fabs(FORM_CRFD(inst), FORM_FB(inst), FORM_RC(inst), m_branch_proc.m_cr,
                              m_system_proc.m_msr, m_system_proc.m_srr1);
            break;
        case TableSubOpcode63::MFFS:
            m_float_proc.mffs(FORM_FS(inst), FORM_RC(inst), m_branch_proc.m_cr, m_system_proc.m_msr,
                              m_system_proc.m_srr1);
            break;
        case TableSubOpcode63::MTFS:
        default:
            internalInvalidCB(PROC_INVALID_MSG(SystemDolphin, unknown,
                                               "Attempted to evaluate unknown instruction!"));
            break;
        }
        return m_system_proc.m_pc + 4;
    }

}  // namespace Toolbox::Interpreter