# Skills Layer

Purpose:
- Define built-in agent skills.
- Map explicit skill names, smart feature types, and content hints to a skill.
- Declare default actions and permission flags for each skill.

Primary files:
- `registry.py`

Coupling rule:
- Skill definitions should describe intent and allowed capabilities, not execute tools directly.
