// BarrierOption: supports Down/Up & In/Out varieties. Also supports Call/Put with optional rebate when knocked out.
#pragma once
#include "PathPayoff.hpp"
#include <algorithm>
#include <optional>

enum class BarrierType
{
  DownAndOut,
  DownAndIn,
  UpAndOut,
  UpAndIn
};
enum class OptionType
{
  Call,
  Put
};

class BarrierOption : public PathPayoff
{
public:
  // If rebate is provided, it is paid when the option knocks out (for Out types).
  BarrierOption(double strike, double barrier, BarrierType type,
                OptionType optType = OptionType::Call,
                std::optional<double> rebate = std::nullopt)
      : m_strike(strike), m_barrier(barrier), m_type(type), m_optType(optType), m_rebate(rebate) {}

  double operator()(const std::vector<double> &path) const override
  {
    if (path.empty())
      return 0.0;

    bool hit = false;
    for (double price : path)
    {
      if (isHit(price))
      {
        hit = true;
        break;
      }
    }

    // Out: knocked out -> return rebate (or 0)
    if (isOut() && hit)
    {
      return m_rebate.value_or(0.0);
    }

    // In: must be hit at some point to be active
    if (isIn() && !hit)
    {
      return 0.0;
    }

    double spotT = path.back();
    // Payoff depends on option type
    if (m_optType == OptionType::Call)
    {
      return std::max(spotT - m_strike, 0.0);
    }
    else
    {
      return std::max(m_strike - spotT, 0.0);
    }
  }

  std::string name() const override
  {
    std::string typeStr;
    switch (m_type)
    {
    case BarrierType::DownAndOut:
      typeStr = "Down-and-Out";
      break;
    case BarrierType::DownAndIn:
      typeStr = "Down-and-In";
      break;
    case BarrierType::UpAndOut:
      typeStr = "Up-and-Out";
      break;
    case BarrierType::UpAndIn:
      typeStr = "Up-and-In";
      break;
    }
    std::string optStr = (m_optType == OptionType::Call) ? "Call" : "Put";
    return typeStr + " " + optStr;
  }

private:
  bool isHit(double price) const
  {
    switch (m_type)
    {
    case BarrierType::DownAndOut:
    case BarrierType::DownAndIn:
      return price <= m_barrier;
    case BarrierType::UpAndOut:
    case BarrierType::UpAndIn:
      return price >= m_barrier;
    }
    return false;
  }

  bool isOut() const { return m_type == BarrierType::DownAndOut || m_type == BarrierType::UpAndOut; }
  bool isIn() const { return m_type == BarrierType::DownAndIn || m_type == BarrierType::UpAndIn; }

  double m_strike;
  double m_barrier;
  BarrierType m_type;
  OptionType m_optType;
  std::optional<double> m_rebate;
};
