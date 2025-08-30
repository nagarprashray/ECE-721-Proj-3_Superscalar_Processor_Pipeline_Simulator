#include "renamer.h"

renamer::renamer(uint64_t n_log_regs, uint64_t n_phys_regs, uint64_t n_branches, uint64_t n_active) {
    assert(n_phys_regs > n_log_regs);
    assert(n_branches >= 1 && n_branches <= 64);
    assert(n_active > 0);

    RMT.resize(n_log_regs);
    AMT.resize(n_log_regs);
    FL.resize(n_phys_regs - n_log_regs);
    AL.resize(n_active);
    PRF.resize(n_phys_regs);
    RDY_BIT.resize(n_phys_regs);
    BR_CHK.resize(n_branches);

    fl_head = 0; fl_tail = 0; fl_h_phase = 0; fl_t_phase = 1;
    al_head = 0; al_tail = 0; al_h_phase = 0; al_t_phase = 0;

    for (int i = 0; i < RMT.size(); i++) {
        AMT[i].committed_phy_reg_mapping = i;
        RMT[i].phy_reg_mapping = i;
    }

    for (int i = 0; i < PRF.size(); i++) {
        RDY_BIT[i].rdy = 1;
    }

    free_list_entries = FL.size();
    int free_phy_reg_entry = n_log_regs;
    for (int i = 0; i < FL.size(); i++) {
        FL[i].phy_reg_num = free_phy_reg_entry++;
    }

    active_list_empty_entries = AL.size();
    for (int i = 0; i < AL.size(); i++) {
        AL[i].dest_flag = 1;
        AL[i].logical_reg_num = 0;
        AL[i].phy_reg_num = 0;
        AL[i].complete_bit = 0;
        AL[i].execp_bit = 0;
        AL[i].load_viol_bit = 0;
        AL[i].br_mispred_bit = 0;
        AL[i].val_mispred_bit = 0;
        AL[i].load_flg = 0;
        AL[i].str_flg = 0;
        AL[i].br_flg = 0;
        AL[i].amo_flg = 0;
        AL[i].csr_flg = 0;
        AL[i].pc = 0;
    }

    GBM = 0;
    for (int i = 0; i < BR_CHK.size(); i++) {
        BR_CHK[i].SMT.resize(n_log_regs);
        BR_CHK[i].gbm_check = GBM;
    }
}

renamer::~renamer() {}

bool renamer::is_FL_empty() {
    return (fl_head == fl_tail) && (fl_h_phase == fl_t_phase);
}

bool renamer::is_FL_full() {
    return (fl_head == fl_tail) && (fl_h_phase != fl_t_phase);
}

bool renamer::is_AL_empty() {
    return (al_head == al_tail) && (al_h_phase == al_t_phase);
}

bool renamer::is_AL_full() {
    return (al_head == al_tail) && (al_h_phase != al_t_phase);
}

int renamer::avail_FL_entries() {
    if (is_FL_full()) return FL.size();
    if (is_FL_empty()) return 0;
    if (fl_head < fl_tail) return fl_tail - fl_head;
    return FL.size() - (fl_head - fl_tail);
}

int renamer::avail_AL_entries() {
    if (is_AL_full()) return 0;
    if (is_AL_empty()) return AL.size();
    if (al_head < al_tail) return AL.size() - (al_tail - al_head);
    return al_head - al_tail;
}

bool renamer::stall_reg(uint64_t bundle_dst) {
    return avail_FL_entries() < bundle_dst;
}

bool renamer::stall_branch(uint64_t bundle_branch) {
    int occupied_branches = 0;
    int mask = 1;
    int temp_gbm = GBM;

    for (int i = 0; i < BR_CHK.size(); i++) {
        if (temp_gbm & mask) occupied_branches++;
        temp_gbm >>= 1;
    }

    return (BR_CHK.size() - occupied_branches) < bundle_branch;
}

uint64_t renamer::get_branch_mask() {
    return GBM;
}

uint64_t renamer::rename_rsrc(uint64_t log_reg) {
    return RMT[log_reg].phy_reg_mapping;
}

uint64_t renamer::rename_rdst(uint64_t log_reg) {
    int new_dest = FL[fl_head].phy_reg_num;
    fl_head = (fl_head + 1) % FL.size();
    if (fl_head == 0) fl_h_phase = !fl_h_phase;
    free_list_entries--;

    RMT[log_reg].phy_reg_mapping = new_dest;
    return new_dest;
}

uint64_t renamer::checkpoint() {
    assert(!stall_branch(1));

    uint64_t branch_id = set_GBM_bit();
    for (int i = 0; i < RMT.size(); i++) {
        BR_CHK[branch_id].SMT[i].phy_reg_mapping = RMT[i].phy_reg_mapping;
    }

    BR_CHK[branch_id].head = fl_head;
    BR_CHK[branch_id].head_ph = fl_h_phase;
    return branch_id;
}

bool renamer::stall_dispatch(uint64_t bundle_inst) {
    return avail_AL_entries() < bundle_inst;
}

uint64_t renamer::dispatch_inst(bool dest_valid, uint64_t log_reg, uint64_t phys_reg, bool load, bool store, bool branch, bool amo, bool csr, uint64_t PC) {
    assert(avail_AL_entries() > 0);

    AL[al_tail].dest_flag = dest_valid;
    AL[al_tail].logical_reg_num = log_reg;
    AL[al_tail].phy_reg_num = phys_reg;
    AL[al_tail].load_flg = load;
    AL[al_tail].str_flg = store;
    AL[al_tail].br_flg = branch;
    AL[al_tail].amo_flg = amo;
    AL[al_tail].csr_flg = csr;
    AL[al_tail].pc = PC;
    AL[al_tail].complete_bit = 0;
    AL[al_tail].execp_bit = 0;
    AL[al_tail].load_viol_bit = 0;
    AL[al_tail].br_mispred_bit = 0;
    AL[al_tail].val_mispred_bit = 0;

    int temp_tail = al_tail;
    al_tail = (al_tail + 1) % AL.size();
    if (al_tail == 0) al_t_phase = !al_t_phase;
    active_list_empty_entries--;

    return temp_tail;
}

bool renamer::is_ready(uint64_t phys_reg) {
    return RDY_BIT[phys_reg].rdy == 1;
}

void renamer::clear_ready(uint64_t phys_reg) {
    RDY_BIT[phys_reg].rdy = 0;
}

uint64_t renamer::read(uint64_t phys_reg) {
    return PRF[phys_reg].value;
}

void renamer::set_ready(uint64_t phys_reg) {
    RDY_BIT[phys_reg].rdy = 1;
}

void renamer::write(uint64_t phys_reg, uint64_t value) {
    PRF[phys_reg].value = value;
}

void renamer::set_complete(uint64_t AL_index) {
    AL[AL_index].complete_bit = 1;
}

void renamer::resolve(uint64_t AL_index, uint64_t branch_ID, bool correct) {
    if (correct) {
        clear_GBM_bit(branch_ID);
        for (int i = 0; i < BR_CHK.size(); i++) {
            uint64_t new_gbm = clear_bit(branch_ID, BR_CHK[i].gbm_check);
            BR_CHK[i].gbm_check = new_gbm;
        }
    } else {
        GBM = BR_CHK[branch_ID].gbm_check;
        for (int i = 0; i < RMT.size(); i++) {
            RMT[i].phy_reg_mapping = BR_CHK[branch_ID].SMT[i].phy_reg_mapping;
        }

        fl_head = BR_CHK[branch_ID].head;
        fl_h_phase = BR_CHK[branch_ID].head_ph;

        al_tail = (AL_index + 1) % AL.size();
        if (al_head < al_tail) al_t_phase = al_h_phase;
        if (al_head > al_tail) al_t_phase = !al_h_phase;
    }
}

bool renamer::precommit(bool &completed, bool &exception, bool &load_viol, bool &br_misp, bool &val_misp, bool &load, bool &store, bool &branch, bool &amo, bool &csr, uint64_t &PC) {
    if (is_AL_empty()) return false;

    completed = AL[al_head].complete_bit;
    exception = AL[al_head].execp_bit;
    load_viol = AL[al_head].load_viol_bit;
    br_misp = AL[al_head].br_mispred_bit;
    val_misp = AL[al_head].val_mispred_bit;
    load = AL[al_head].load_flg;
    store = AL[al_head].str_flg;
    branch = AL[al_head].br_flg;
    amo = AL[al_head].amo_flg;
    csr = AL[al_head].csr_flg;
    PC = AL[al_head].pc;

    return true;
}

void renamer::commit() {
    assert(!is_AL_empty());
    assert(AL[al_head].complete_bit == 1);
    assert(AL[al_head].execp_bit == 0);
    assert(AL[al_head].load_viol_bit == 0);

    if (AL[al_head].dest_flag == 1) {
        FL[fl_tail].phy_reg_num = AMT[AL[al_head].logical_reg_num].committed_phy_reg_mapping;
        fl_tail = (fl_tail + 1) % FL.size();
        if (fl_tail == 0) fl_t_phase = !fl_t_phase;
        free_list_entries++;

        AMT[AL[al_head].logical_reg_num].committed_phy_reg_mapping = AL[al_head].phy_reg_num;
    }

    AL[al_head].complete_bit = 0;
    AL[al_head].br_flg = 0;
    al_head = (al_head + 1) % AL.size();
    if (al_head == 0) al_h_phase = !al_h_phase;
    active_list_empty_entries++;
}

void renamer::squash() {
    for (int i = 0; i < RMT.size(); i++) {
        RMT[i].phy_reg_mapping = AMT[i].committed_phy_reg_mapping;
    }

    fl_head = fl_tail;
    fl_h_phase = !fl_t_phase;
    free_list_entries = FL.size();

    al_tail = al_head;
    al_t_phase = al_h_phase;
    active_list_empty_entries = AL.size();

    GBM = 0;
}

void renamer::set_exception(uint64_t AL_index) {
    AL[AL_index].execp_bit = 1;
}

void renamer::set_load_violation(uint64_t AL_index) {
    AL[AL_index].load_viol_bit = 1;
}

void renamer::set_branch_misprediction(uint64_t AL_index) {
    AL[AL_index].br_mispred_bit = 1;
}

void renamer::set_value_misprediction(uint64_t AL_index) {
    AL[AL_index].val_mispred_bit = 1;
}

bool renamer::get_exception(uint64_t AL_index) {
    return AL[AL_index].execp_bit == 1;
}

uint64_t renamer::set_GBM_bit() {
    int pos = -1;
    int temp_gbm = GBM, mask = 1;
    for (int i = 0; i < BR_CHK.size(); i++) {
        int gbm_bit = (temp_gbm & mask);
        if (gbm_bit == 0) {
            pos = i;
            break;
        }
        temp_gbm >>= 1;
    }

    assert(pos > -1);

    int set_bit_mask = 1 << pos;
    BR_CHK[pos].gbm_check = GBM;
    GBM = (set_bit_mask | GBM);
    return pos;
}

void renamer::clear_GBM_bit(uint64_t branch_id) {
    uint64_t mask = ~(1 << branch_id);
    GBM = GBM & mask;
}

uint64_t renamer::clear_bit(uint64_t branch_id, uint64_t chk_gbm) {
    uint64_t mask = ~(1 << branch_id);
    return chk_gbm & mask;
}