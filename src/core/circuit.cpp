#include "circuit.hpp"

#include "current_source.hpp"
#include "voltage_source.hpp"

void Circuit::extractNodes() {
  std::set<Node*> nodeSet;
  const int component_types = 5;
  for (int i = 0; i < component_types; i++) {
    std::vector<Component*>* components;
    switch (i) {
      case 0:
        components = &resistors_;
        break;
      case 1:
        components = &capacitors_;
        break;
      case 2:
        components = &inductors_;
        break;
      case 3:
        components = &voltage_sources_;
        break;
      case 4:
        components = &current_sources_;
        break;
    }
    for (auto c : *components) {
      if (c->GetNegNode() && c->GetPosNode()->GetName() == "GND")
        hasGND_ = true;
      if (c->GetPosNode() && c->GetNegNode()->GetName() == "GND")
        hasGND_ = true;
      if (c->GetPosNode() && c->GetPosNode()->GetName() != "GND")
        nodeSet.insert(c->GetPosNode());
      if (c->GetNegNode() && c->GetNegNode()->GetName() != "GND")
        nodeSet.insert(c->GetNegNode());
    }
    for (auto vcvs : VCVSs_) {
      if (vcvs->GetPosNode() && vcvs->GetPosNode()->GetName() != "GND")
        nodeSet.insert(vcvs->GetPosNode());
      if (vcvs->GetNegNode() && vcvs->GetNegNode()->GetName() != "GND")
        nodeSet.insert(vcvs->GetNegNode());
    }
    for (auto vccs : VCCSs_) {
      if (vccs->GetPosNode() && vccs->GetPosNode()->GetName() != "GND")
        nodeSet.insert(vccs->GetPosNode());
      if (vccs->GetNegNode() && vccs->GetNegNode()->GetName() != "GND")
        nodeSet.insert(vccs->GetNegNode());
    }
    for (auto cccs : CCCSs_) {
      if (cccs->GetPosNode() && cccs->GetPosNode()->GetName() != "GND")
        nodeSet.insert(cccs->GetPosNode());
      if (cccs->GetNegNode() && cccs->GetNegNode()->GetName() != "GND")
        nodeSet.insert(cccs->GetNegNode());
    }
    for (auto ccvs : CCVSs_) {
      if (ccvs->GetPosNode() && ccvs->GetPosNode()->GetName() != "GND")
        nodeSet.insert(ccvs->GetPosNode());
      if (ccvs->GetNegNode() && ccvs->GetNegNode()->GetName() != "GND")
        nodeSet.insert(ccvs->GetNegNode());
    }
  }
  nodes_ = std::set<Node*, NodeComparator>(nodeSet.begin(), nodeSet.end());
}

std::vector<Component*> Circuit::GetComponents() const {
  std::vector<Component*> all;

  all.insert(all.end(), resistors_.begin(), resistors_.end());
  all.insert(all.end(), capacitors_.begin(), capacitors_.end());
  all.insert(all.end(), inductors_.begin(), inductors_.end());
  all.insert(all.end(), voltage_sources_.begin(), voltage_sources_.end());
  all.insert(all.end(), current_sources_.begin(), current_sources_.end());

  all.insert(all.end(), VCVSs_.begin(), VCVSs_.end());
  all.insert(all.end(), VCCSs_.begin(), VCCSs_.end());
  all.insert(all.end(), CCCSs_.begin(), CCCSs_.end());
  all.insert(all.end(), CCVSs_.begin(), CCVSs_.end());

  all.insert(all.end(), opamps_.begin(), opamps_.end());

  return all;
};

Circuit::Circuit(std::vector<Component*>& components) {
  for (auto* comp : components) {
    if (dynamic_cast<Resistor*>(comp))
      resistors_.push_back(comp);
    else if (dynamic_cast<Capacitor*>(comp))
      capacitors_.push_back(comp);
    else if (dynamic_cast<Inductor*>(comp))
      inductors_.push_back(comp);
    else if (dynamic_cast<VoltageSource*>(comp))
      voltage_sources_.push_back(comp);
    else if (dynamic_cast<CurrentSource*>(comp))
      current_sources_.push_back(comp);
  }
}

void Circuit::AddResistor(const std::string& n, double r, Node* n1, Node* n2) {
  resistors_.push_back(new Resistor(n, r, n1, n2));
}

void Circuit::AddCapacitor(const std::string& n, double c, Node* n1, Node* n2) {
  capacitors_.push_back(new Capacitor(n, c, n1, n2));
}

void Circuit::AddInductor(const std::string& n, double L, Node* n1, Node* n2) {
  inductors_.push_back(new Inductor(n, L, n1, n2));
}

void Circuit::AddVoltageSource(const std::string& n, double v, Node* n1,
                               Node* n2) {
  voltage_sources_.push_back(new VoltageSource(n, v, n1, n2));
}

void Circuit::AddCurrentSource(const std::string& n, double i, Node* n1,
                               Node* n2) {
  current_sources_.push_back(new CurrentSource(n, i, n1, n2));
}

void Circuit::AddVCVS(const std::string& n, double A, Node* n1, Node* n2,
                      Node* depn1, Node* depn2) {
  VCVSs_.push_back(new VCVS(n, A, n1, n2, depn1, depn2));
}

void Circuit::AddVCCS(const std::string& n, double gm, Node* n1, Node* n2,
                      Node* depn1, Node* depn2) {
  VCCSs_.push_back(new VCCS(n, gm, n1, n2, depn1, depn2));
}

void Circuit::AddCCCS(const std::string& n, double A, Node* n1, Node* n2,
                      ShortCircuit* sc) {
  CCCSs_.push_back(new CCCS(n, A, n1, n2, sc));
}

void Circuit::AddCCVS(const std::string& n, double Rm, Node* n1, Node* n2,
                      ShortCircuit* sc) {
  CCVSs_.push_back(new CCVS(n, Rm, n1, n2, sc));
}

ShortCircuit* Circuit::AddShortCircuit(const std::string& n, Node* n1,
                                       Node* n2) {
  ShortCircuit* sc = new ShortCircuit(n, n1, n2);
  voltage_sources_.push_back(sc);
  return sc;
}

void Circuit::AddOpAmp(const std::string& n, Node* n1, Node* n2,
                       Node* outnode) {
  opamps_.push_back(new OpAmp(n, n1, n2, outnode));
}

Circuit::~Circuit() {
  for (auto* r : resistors_) delete r;
  for (auto* c : capacitors_) delete c;
  for (auto* L : inductors_) delete L;
  for (auto* v : voltage_sources_) delete v;
  for (auto* i : current_sources_) delete i;
  for (auto* vcvs : VCVSs_) delete vcvs;
  for (auto* vccs : VCCSs_) delete vccs;
  for (auto* cccs : CCCSs_) delete cccs;
  for (auto* ccvs : CCVSs_) delete ccvs;
  for (auto* oa : opamps_) delete oa;
}

Node* Circuit::GetOrCreateNode(const std::string& name) {
  if (name == "0" && GND_ != nullptr) {
    return GND_;
  }
  Node temp(-1, name);
  auto it = nodes_.find(&temp);
  if (it != nodes_.end()) {
    return *it;
  }
  Node* newNode = new Node(name);
  if (name != "0") {
    nodes_.insert(newNode);
  } else {
    GND_ = newNode;
    GND_->name_ = "GND";
  }
  return newNode;
}

bool Circuit::HasGND() const { return hasGND_; }