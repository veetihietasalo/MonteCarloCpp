#pragma once
#include <algorithm>
#include <string>
#include <optional>

// Abstract base class for Option Payoff
class Payoff
{
public:
  virtual ~Payoff() = default;
  virtual double operator()(double spot) const = 0;
  virtual std::string name() const = 0;
  // For some payoff types (calls/puts), the strike is meaningful.
  // Default returns an empty optional (unknown).
  virtual std::optional<double> strike() const { return std::nullopt; }
};

// European Call Option Payoff
// `final`: nothing can derive from it, so a call through a PayoffCall (not a Payoff)
// can be resolved at compile time and inlined.
class PayoffCall final : public Payoff
{
public:
  PayoffCall(double strike) : m_strike(strike) {}

  double operator()(double spot) const override
  {
    return std::max(spot - m_strike, 0.0);
  }

  std::string name() const override { return "European Call"; }
  std::optional<double> strike() const override { return m_strike; }

private:
  double m_strike;
};

// European Put Option Payoff
class PayoffPut final : public Payoff
{
public:
  PayoffPut(double strike) : m_strike(strike) {}

  double operator()(double spot) const override
  {
    return std::max(m_strike - spot, 0.0);
  }

  std::string name() const override { return "European Put"; }
  std::optional<double> strike() const override { return m_strike; }

private:
  double m_strike;
};
