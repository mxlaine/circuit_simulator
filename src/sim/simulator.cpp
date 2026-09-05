#include "simulator.hpp"

#include <cmath>
#include <complex>
#include <iostream>

#include "capacitor.hpp"
#include "cccs.hpp"
#include "ccvs.hpp"
#include "circuit.hpp"
#include "component.hpp"
#include "current_source.hpp"
#include "node.hpp"
#include "resistor.hpp"
#include "short_circuit.hpp"
#include "vccs.hpp"
#include "vcvs.hpp"
#include "voltage_source.hpp"

using namespace Eigen;

Simulator::Simulator() : circuit_(nullptr) {}
Simulator::Simulator(Circuit* circuit) : circuit_(circuit) {}

int Simulator::FindNodeIndex(Node* node) {
  if (!node || !circuit_) return -1;
  auto nodes = circuit_->GetNodes();
  for (int i = 0; i < (int)nodes.size(); i++)
    if (nodes[i] == node) return i;
  return -1;
}

int Simulator::GetBranchIndexForShortCircuit(ShortCircuit* sc) {
  auto v_sources = circuit_->GetVoltageSources();
  for (int i = 0; i < (int)v_sources.size(); i++)
    if (v_sources[i] == sc) return i;
  return -1;
}

void Simulator::FillGCMatrix(MatrixXd& m, std::vector<Component*>& components) {
  auto nodes = circuit_->GetNodes();
  int n = nodes.size();
  m = MatrixXd::Zero(n, n);
  auto find_index = [&](Node* node) -> int {
    if (!node) return -1;
    for (int i = 0; i < n; i++)
      if (nodes[i] == node) return i;
    return -1;
  };
  for (auto c : components) {
    if (!c) continue;
    int pi = find_index(c->GetPosNode());
    int ni = find_index(c->GetNegNode());
    if (pi == -1 && ni == -1) continue;
    double val = c->GetValue();
    if (pi >= 0) m(pi, pi) += val;
    if (ni >= 0) m(ni, ni) += val;
    if (pi >= 0 && ni >= 0) {
      m(pi, ni) -= val;
      m(ni, pi) -= val;
    }
  }
}

void Simulator::AddVCCSToG(MatrixXd& G) {
  auto nodes = circuit_->GetNodes();
  auto vccss = circuit_->GetVCCSs();
  auto find_index = [&](Node* n) -> int {
    if (!n) return -1;
    for (int i = 0; i < (int)nodes.size(); i++)
      if (nodes[i] == n) return i;
    return -1;
  };
  for (auto v : vccss) {
    int p = find_index(v->GetPosNode());
    int n_ = find_index(v->GetNegNode());
    int dp = find_index(v->GetPosDepNode());
    int dn = find_index(v->GetNegDepNode());
    double gm = v->GetValue();
    if (n_ >= 0 && dp >= 0) G(n_, dp) += gm;
    if (n_ >= 0 && dn >= 0) G(n_, dn) -= gm;
    if (p >= 0 && dp >= 0) G(p, dp) -= gm;
    if (p >= 0 && dn >= 0) G(p, dn) += gm;
  }
}

void Simulator::FillBMatrix(MatrixXd& B) {
  auto nodes = circuit_->GetNodes();
  auto v_sources = circuit_->GetVoltageSources();
  auto inductors = circuit_->GetInductors();

  int n = nodes.size();
  int v = v_sources.size();

  B = MatrixXd::Zero(n, v);

  auto find_index = [&](Node* node) -> int {
    if (!node) return -1;
    for (int i = 0; i < n; i++)
      if (nodes[i] == node) return i;
    return -1;
  };

  for (int i = 0; i < v; i++) {
    auto vs = v_sources[i];
    if (!vs) continue;
    int pi = find_index(vs->GetPosNode());
    int ni = find_index(vs->GetNegNode());
    if (pi >= 0) B(pi, i) = 1.0;
    if (ni >= 0) B(ni, i) = -1.0;
  }
}

void Simulator::FillAMatrix(MatrixXd& A) {
  auto nodes = circuit_->GetNodes();
  auto resistors = circuit_->GetResistors();
  auto v_sources = circuit_->GetVoltageSources();
  auto inductors = circuit_->GetInductors();
  auto vcvss = circuit_->GetVCVSs();
  auto ccvss = circuit_->GetCCVSs();
  auto cccss = circuit_->GetCCCSs();
  auto opamps = circuit_->GetOpAmps();

  int n_size = nodes.size();
  int v_size = v_sources.size();
  int vcvs_size = vcvss.size();
  int ccvs_size = ccvss.size();
  int opamp_size = opamps.size();
  int cols = n_size + v_size + vcvs_size + ccvs_size + opamp_size;

  Eigen::MatrixXd G;
  FillGCMatrix(G, resistors);
  AddVCCSToG(G);

  Eigen::MatrixXd B;
  FillBMatrix(B);

  if (B.rows() != n_size || B.cols() != v_size)
    B = Eigen::MatrixXd::Zero(n_size, v_size);

  A = Eigen::MatrixXd::Zero(cols, cols);

  if (G.rows() == n_size && G.cols() == n_size)
    A.block(0, 0, n_size, n_size) = G;

  if (B.rows() == n_size && B.cols() == v_size) {
    A.block(0, n_size, n_size, v_size) = B;
    A.block(n_size, 0, v_size, n_size) = B.transpose();
  }

  auto find_index = [&](Node* n_) -> int {
    if (!n_) return -1;
    for (int i = 0; i < (int)nodes.size(); i++)
      if (nodes[i] == n_) return i;
    return -1;
  };

  for (auto cccs : cccss) {
    int p = find_index(cccs->GetPosNode());
    int n_ = find_index(cccs->GetNegNode());
    int k = GetBranchIndexForShortCircuit(cccs->GetShortCircuit());
    double gain = cccs->GetValue();

    if (k >= 0) {
      int branch_col = n_size + k;
      if (p >= 0) A(p, branch_col) -= gain;
      if (n_ >= 0) A(n_, branch_col) += gain;
    }
  }

  int current_extra_row = n_size + v_size;

  for (int k = 0; k < (int)vcvss.size(); k++) {
    auto vcvs = vcvss[k];
    int row = current_extra_row + k;

    int np = vcvs->GetPosNode() ? vcvs->GetPosNode()->GetIndex() : -1;
    int nn = vcvs->GetNegNode() ? vcvs->GetNegNode()->GetIndex() : -1;
    int nc_p = vcvs->GetPosDepNode() ? vcvs->GetPosDepNode()->GetIndex() : -1;
    int nc_n = vcvs->GetNegDepNode() ? vcvs->GetNegDepNode()->GetIndex() : -1;
    double gain = vcvs->GetValue();

    if (np > 0) A(row, np - 1) = 1.0f;
    if (nn > 0) A(row, nn - 1) = -1.0f;
    if (nc_p > 0) A(row, nc_p - 1) -= gain;
    if (nc_n > 0) A(row, nc_n - 1) += gain;

    if (np > 0) A(np - 1, row) = 1.0f;
    if (nn > 0) A(nn - 1, row) = -1.0f;
  }

  current_extra_row += vcvs_size;

  for (int k = 0; k < (int)ccvss.size(); k++) {
    auto ccvs = ccvss[k];
    int row = current_extra_row + k;

    int np = ccvs->GetPosNode() ? ccvs->GetPosNode()->GetIndex() : -1;
    int nn = ccvs->GetNegNode() ? ccvs->GetNegNode()->GetIndex() : -1;
    double gain = ccvs->GetValue();

    int ctrl_branch = GetBranchIndexForShortCircuit(ccvs->GetShortCircuit());

    if (np > 0) A(row, np - 1) = 1.0f;
    if (nn > 0) A(row, nn - 1) = -1.0f;
    if (ctrl_branch >= 0) A(row, n_size + ctrl_branch) = -gain;

    if (np > 0) A(np - 1, row) = 1.0f;
    if (nn > 0) A(nn - 1, row) = -1.0f;
  }

  current_extra_row += ccvs_size;

  for (int k = 0; k < (int)opamps.size(); k++) {
    auto opamp = opamps[k];
    int row = current_extra_row + k;

    int n_plus = opamp->GetPosNode() ? opamp->GetPosNode()->GetIndex() : -1;
    int n_minus = opamp->GetNegNode() ? opamp->GetNegNode()->GetIndex() : -1;
    int n_out = opamp->GetOutNode() ? opamp->GetOutNode()->GetIndex() : -1;

    if (n_plus > 0) A(row, n_plus - 1) = 1.0f;
    if (n_minus > 0) A(row, n_minus - 1) = -1.0f;

    if (n_out > 0) A(n_out - 1, row) = 1.0f;
  }
}

void Simulator::SolveDC() {
  CheckCircuitTopology(false);
  auto nodes = circuit_->GetNodes();
  auto resistors = circuit_->GetResistors();
  auto v_sources = circuit_->GetVoltageSources();
  auto vcvss = circuit_->GetVCVSs();
  auto ccvss = circuit_->GetCCVSs();
  auto cccss = circuit_->GetCCCSs();
  auto opamps = circuit_->GetOpAmps();

  int n_size = nodes.size();
  int v_size = v_sources.size();
  int vcvs_size = vcvss.size();
  int ccvs_size = ccvss.size();
  int opamp_size = opamps.size();
  int rows = n_size + v_size + vcvs_size + ccvs_size + opamp_size;

  Eigen::MatrixXd A;
  FillAMatrix(A);

  if (A.rows() == 0) return;

  Eigen::VectorXd z = Eigen::VectorXd::Zero(rows);

  auto find_index = [&](Node* node) -> int {
    if (!node) return -1;
    for (int i = 0; i < n_size; ++i)
      if (nodes[i] == node) return i;
    return -1;
  };

  for (auto cs : circuit_->GetCurrentSources()) {
    if (!cs) continue;
    double I = cs->GetValue();
    int pi = find_index(cs->GetPosNode());
    int ni = find_index(cs->GetNegNode());
    if (pi >= 0) z(pi) -= I;
    if (ni >= 0) z(ni) += I;
  }

  for (int i = 0; i < v_size; ++i) {
    z(n_size + i) = v_sources[i] ? v_sources[i]->GetValue() : 0.0f;
  }

  Eigen::VectorXd x = A.colPivHouseholderQr().solve(z);

  for (int i = 0; i < n_size; ++i) nodes[i]->voltage_ = x(i);

  for (int i = 0; i < v_size; ++i) {
    if (i < (int)v_sources.size() && v_sources[i])
      v_sources[i]->current_ = x(n_size + i);
  }

  for (int i = 0; i < vcvs_size; ++i) {
    if (i < (int)vcvss.size() && vcvss[i])
      vcvss[i]->current_ = x(n_size + v_size + i);
  }

  for (int i = 0; i < ccvs_size; ++i) {
    if (i < (int)ccvss.size() && ccvss[i])
      ccvss[i]->current_ = x(n_size + v_size + vcvs_size + i);
  }

  for (int i = 0; i < opamp_size; ++i) {
    if (i < (int)opamps.size() && opamps[i])
      opamps[i]->current_ = x(n_size + v_size + vcvs_size + ccvs_size + i);
  }

  for (auto cccs : cccss) {
    if (!cccs) continue;
    ShortCircuit* sc = cccs->GetShortCircuit();
    if (!sc) continue;

    int ctrl_idx = GetBranchIndexForShortCircuit(sc);
    if (ctrl_idx >= 0) {
      double control_current = x(n_size + ctrl_idx);
      cccs->current_ = cccs->GetValue() * control_current;
    }
  }

  for (auto r : resistors) {
    if (!r) continue;
    double vp = r->GetPosNode() ? r->GetPosNode()->voltage_ : 0.0;
    double vn = r->GetNegNode() ? r->GetNegNode()->voltage_ : 0.0;
    if (r->GetPosNode() && r->GetPosNode()->GetName() == "GND") vp = 0.0;
    if (r->GetNegNode() && r->GetNegNode()->GetName() == "GND") vn = 0.0;
    r->current_ = (vp - vn) * r->GetValue();
  }

  for (auto c : circuit_->GetCapacitors()) {
    if (c) c->current_ = 0.0;
  }
}

void Simulator::SolveAC(double frequency) {
  CheckCircuitTopology(true);

  auto nodes = circuit_->GetNodes();
  auto resistors = circuit_->GetResistors();
  auto capacitors = circuit_->GetCapacitors();
  auto inductors = circuit_->GetInductors();
  auto v_sources = circuit_->GetVoltageSources();
  auto vcvss = circuit_->GetVCVSs();
  auto ccvss = circuit_->GetCCVSs();
  auto cccss = circuit_->GetCCCSs();
  auto opamps = circuit_->GetOpAmps();

  int n_size = nodes.size();
  int v_size = v_sources.size();
  int vcvs_size = vcvss.size();
  int ccvs_size = ccvss.size();
  int opamp_size = opamps.size();
  int rows = n_size + v_size + vcvs_size + ccvs_size + opamp_size;

  double omega = 2.0 * M_PI * frequency;
  std::complex<double> j(0.0, 1.0);

  MatrixXcd G = MatrixXcd::Zero(n_size, n_size);

  auto find_index = [&](Node* node) -> int {
    if (!node) return -1;
    for (int i = 0; i < n_size; ++i)
      if (nodes[i] == node) return i;
    return -1;
  };

  for (auto r : resistors) {
    if (!r) continue;
    int pi = find_index(r->GetPosNode());
    int ni = find_index(r->GetNegNode());
    if (pi == -1 && ni == -1) continue;

    double G_val = r->GetValue();  // Conductance

    if (pi >= 0) G(pi, pi) += G_val;
    if (ni >= 0) G(ni, ni) += G_val;
    if (pi >= 0 && ni >= 0) {
      G(pi, ni) -= G_val;
      G(ni, pi) -= G_val;
    }
  }

  for (auto c : capacitors) {
    if (!c) continue;
    int pi = find_index(c->GetPosNode());
    int ni = find_index(c->GetNegNode());
    if (pi == -1 && ni == -1) continue;

    std::complex<double> Y_C = j * omega * c->GetValue();

    if (pi >= 0) G(pi, pi) += Y_C;
    if (ni >= 0) G(ni, ni) += Y_C;
    if (pi >= 0 && ni >= 0) {
      G(pi, ni) -= Y_C;
      G(ni, pi) -= Y_C;
    }
  }

  for (auto l : inductors) {
    if (!l) continue;
    int pi = find_index(l->GetPosNode());
    int ni = find_index(l->GetNegNode());
    if (pi == -1 && ni == -1) continue;

    std::complex<double> Y_L = 1.0 / (j * omega * l->GetValue());

    if (pi >= 0) G(pi, pi) += Y_L;
    if (ni >= 0) G(ni, ni) += Y_L;
    if (pi >= 0 && ni >= 0) {
      G(pi, ni) -= Y_L;
      G(ni, pi) -= Y_L;
    }
  }

  auto vccss = circuit_->GetVCCSs();
  for (auto v : vccss) {
    int p = find_index(v->GetPosNode());
    int n_ = find_index(v->GetNegNode());
    int dp = find_index(v->GetPosDepNode());
    int dn = find_index(v->GetNegDepNode());
    double gm = v->GetValue();

    if (n_ >= 0 && dp >= 0) G(n_, dp) += gm;
    if (n_ >= 0 && dn >= 0) G(n_, dn) -= gm;
    if (p >= 0 && dp >= 0) G(p, dp) -= gm;
    if (p >= 0 && dn >= 0) G(p, dn) += gm;
  }

  MatrixXcd B = MatrixXcd::Zero(n_size, v_size);

  for (int i = 0; i < v_size; i++) {
    auto vs = v_sources[i];
    if (!vs) continue;
    int pi = find_index(vs->GetPosNode());
    int ni = find_index(vs->GetNegNode());
    if (pi >= 0) B(pi, i) = 1.0;
    if (ni >= 0) B(ni, i) = -1.0;
  }

  MatrixXcd A = MatrixXcd::Zero(rows, rows);

  A.block(0, 0, n_size, n_size) = G;

  if (v_size > 0) {
    A.block(0, n_size, n_size, v_size) = B;
    A.block(n_size, 0, v_size, n_size) = B.transpose();
  }

  for (auto cccs : cccss) {
    int p = find_index(cccs->GetPosNode());
    int n_ = find_index(cccs->GetNegNode());
    int k = GetBranchIndexForShortCircuit(cccs->GetShortCircuit());
    double gain = cccs->GetValue();

    if (k >= 0) {
      int branch_col = n_size + k;
      if (p >= 0) A(p, branch_col) -= gain;
      if (n_ >= 0) A(n_, branch_col) += gain;
    }
  }

  int current_extra_row = n_size + v_size;
  for (int k = 0; k < vcvs_size; k++) {
    auto vcvs = vcvss[k];
    int row = current_extra_row + k;

    int np = vcvs->GetPosNode() ? vcvs->GetPosNode()->GetIndex() : -1;
    int nn = vcvs->GetNegNode() ? vcvs->GetNegNode()->GetIndex() : -1;
    int nc_p = vcvs->GetPosDepNode() ? vcvs->GetPosDepNode()->GetIndex() : -1;
    int nc_n = vcvs->GetNegDepNode() ? vcvs->GetNegDepNode()->GetIndex() : -1;
    double gain = vcvs->GetValue();

    if (np > 0) A(row, np - 1) = 1.0;
    if (nn > 0) A(row, nn - 1) = -1.0;
    if (nc_p > 0) A(row, nc_p - 1) -= gain;
    if (nc_n > 0) A(row, nc_n - 1) += gain;

    if (np > 0) A(np - 1, row) = 1.0;
    if (nn > 0) A(nn - 1, row) = -1.0;
  }

  current_extra_row += vcvs_size;
  for (int k = 0; k < ccvs_size; k++) {
    auto ccvs = ccvss[k];
    int row = current_extra_row + k;

    int np = ccvs->GetPosNode() ? ccvs->GetPosNode()->GetIndex() : -1;
    int nn = ccvs->GetNegNode() ? ccvs->GetNegNode()->GetIndex() : -1;
    double gain = ccvs->GetValue();

    int ctrl_branch = GetBranchIndexForShortCircuit(ccvs->GetShortCircuit());

    if (np > 0) A(row, np - 1) = 1.0;
    if (nn > 0) A(row, nn - 1) = -1.0;
    if (ctrl_branch >= 0) A(row, n_size + ctrl_branch) = -gain;

    if (np > 0) A(np - 1, row) = 1.0;
    if (nn > 0) A(nn - 1, row) = -1.0;
  }

  current_extra_row += ccvs_size;
  for (int k = 0; k < opamp_size; k++) {
    auto opamp = opamps[k];
    int row = current_extra_row + k;

    int n_plus = opamp->GetPosNode() ? opamp->GetPosNode()->GetIndex() : -1;
    int n_minus = opamp->GetNegNode() ? opamp->GetNegNode()->GetIndex() : -1;
    int n_out = opamp->GetOutNode() ? opamp->GetOutNode()->GetIndex() : -1;

    if (n_plus > 0) A(row, n_plus - 1) = 1.0;
    if (n_minus > 0) A(row, n_minus - 1) = -1.0;

    if (n_out > 0) A(n_out - 1, row) = 1.0;
  }

  if (A.rows() == 0) return;
  VectorXcd z = VectorXcd::Zero(rows);

  for (auto cs : circuit_->GetCurrentSources()) {
    if (!cs) continue;
    std::complex<double> I(cs->GetValue(), 0.0);
    int pi = find_index(cs->GetPosNode());
    int ni = find_index(cs->GetNegNode());
    if (pi >= 0) z(pi) -= I;
    if (ni >= 0) z(ni) += I;
  }

  for (int i = 0; i < v_size; ++i) {
    if (v_sources[i])
      z(n_size + i) = std::complex<double>(v_sources[i]->GetValue(), 0.0);
  }

  VectorXcd x = A.colPivHouseholderQr().solve(z);

  // Voltages
  for (int i = 0; i < n_size; ++i) {
    nodes[i]->voltage_ac_ = x(i);
    nodes[i]->voltage_ = std::abs(x(i));
  }

  // Current calculations
  for (int i = 0; i < v_size; ++i) {
    if (i < (int)v_sources.size() && v_sources[i]) {
      v_sources[i]->complex_current_ = x(n_size + i);
      v_sources[i]->current_ = std::abs(x(n_size + i));
    }
  }

  for (int i = 0; i < vcvs_size; ++i) {
    if (i < (int)vcvss.size() && vcvss[i]) {
      vcvss[i]->complex_current_ = x(n_size + v_size + i);
      vcvss[i]->current_ = std::abs(x(n_size + v_size + i));
    }
  }

  for (int i = 0; i < ccvs_size; ++i) {
    if (i < (int)ccvss.size() && ccvss[i]) {
      ccvss[i]->complex_current_ = x(n_size + v_size + vcvs_size + i);
      ccvss[i]->current_ = std::abs(x(n_size + v_size + vcvs_size + i));
    }
  }

  for (int i = 0; i < opamp_size; ++i) {
    if (i < (int)opamps.size() && opamps[i]) {
      opamps[i]->complex_current_ =
          x(n_size + v_size + vcvs_size + ccvs_size + i);
      opamps[i]->current_ =
          std::abs(x(n_size + v_size + vcvs_size + ccvs_size + i));
    }
  }

  // CCCS current calculation (derived from controlling branch current)
  for (auto cccs : cccss) {
    if (!cccs) continue;
    ShortCircuit* sc = cccs->GetShortCircuit();
    if (!sc) continue;

    int ctrl_idx = GetBranchIndexForShortCircuit(sc);
    if (ctrl_idx >= 0) {
      std::complex<double> control_current = x(n_size + ctrl_idx);
      cccs->complex_current_ = cccs->GetValue() * control_current;
      cccs->current_ = std::abs(cccs->complex_current_);
    }
  }

  for (auto r : resistors) {
    if (!r) continue;
    std::complex<double> vp =
        r->GetPosNode() ? r->GetPosNode()->voltage_ac_ : 0.0;
    std::complex<double> vn =
        r->GetNegNode() ? r->GetNegNode()->voltage_ac_ : 0.0;
    if (r->GetPosNode() && r->GetPosNode()->GetName() == "GND") vp = 0.0;
    if (r->GetNegNode() && r->GetNegNode()->GetName() == "GND") vn = 0.0;

    std::complex<double> v_drop = vp - vn;
    r->complex_current_ = v_drop * r->GetValue();
    r->current_ = std::abs(r->complex_current_);
  }

  for (auto c : capacitors) {
    if (!c) continue;
    std::complex<double> vp =
        c->GetPosNode() ? c->GetPosNode()->voltage_ac_ : 0.0;
    std::complex<double> vn =
        c->GetNegNode() ? c->GetNegNode()->voltage_ac_ : 0.0;
    if (c->GetPosNode() && c->GetPosNode()->GetName() == "GND") vp = 0.0;
    if (c->GetNegNode() && c->GetNegNode()->GetName() == "GND") vn = 0.0;

    std::complex<double> v_drop = vp - vn;
    std::complex<double> admittance = j * omega * c->GetValue();
    c->complex_current_ = v_drop * admittance;
    c->current_ = std::abs(c->complex_current_);
  }

  for (auto l : inductors) {
    if (!l) continue;
    std::complex<double> vp =
        l->GetPosNode() ? l->GetPosNode()->voltage_ac_ : 0.0;
    std::complex<double> vn =
        l->GetNegNode() ? l->GetNegNode()->voltage_ac_ : 0.0;
    if (l->GetPosNode() && l->GetPosNode()->GetName() == "GND") vp = 0.0;
    if (l->GetNegNode() && l->GetNegNode()->GetName() == "GND") vn = 0.0;

    std::complex<double> v_drop = vp - vn;
    std::complex<double> admittance = 1.0 / (j * omega * l->GetValue());
    l->complex_current_ = v_drop * admittance;
    l->current_ = std::abs(l->complex_current_);
  }
}

void Simulator::CheckCircuitTopology(bool isAC = false) {
  if (!circuit_) throw std::runtime_error("Circuit pointer is null.");

  circuit_->extractNodes();
  auto nodes = circuit_->GetNodes();
  auto resistors = circuit_->GetResistors();
  auto v_sources = circuit_->GetVoltageSources();
  auto inductors = circuit_->GetInductors();
  auto capacitors = circuit_->GetCapacitors();

  int n_size = nodes.size();
  if (!isAC) {
    if (!circuit_->GetCapacitors().empty()) {
      std::cout << "DC: Some node have no path to GND due to capacitors."
                << std::endl;
    }
  }
  if (!circuit_->HasGND())
    std::runtime_error("Circuit has no ground reference. Add a GND node.");

  Eigen::MatrixXd G;
  FillGCMatrix(G, resistors);

  Eigen::MatrixXd B;
  FillBMatrix(B);

  int b_cols = B.cols();
  int cols = n_size + b_cols;

  Eigen::MatrixXd A = Eigen::MatrixXd::Zero(cols, cols);

  if (G.rows() == n_size && G.cols() == n_size)
    A.block(0, 0, n_size, n_size) = G;

  A.block(0, n_size, n_size, b_cols) = B;
  A.block(n_size, 0, b_cols, n_size) = B.transpose();

  if (cols == 0)
    throw std::runtime_error("Circuit has no solvable electrical network.");

  Eigen::FullPivLU<Eigen::MatrixXd> lu(A);
  int rank = lu.rank();

  if (rank < cols) {
    std::cout << "Warning: Circuit is singular or topologically invalid "
                 "(floating nodes, only shorts, or unconnected components). "
                 "AC analysis may still be valid."
              << std::endl;
  }
}
