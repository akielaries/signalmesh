def hpa_to_psi(hpa: float) -> float:
  """Convert hectopascals (hPa) to pounds per square inch (PSI).

  Args:
      hpa (float): Pressure in hectopascals.

  Returns:
      float: Pressure in PSI.
  """
  return hpa * 0.014503773773

def hpa_to_inhg(hpa: float) -> float:
  """Convert hectopascals (hPa) to inches of mercury (inHg).

  Args:
      hpa (float): Pressure in hectopascals.

  Returns:
      float: Pressure in inHg.
  """
  return hpa * 0.02953
