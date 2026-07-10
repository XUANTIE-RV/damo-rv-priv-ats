---
name: testplan_translation
description: Translate a Chinese markdown test plan to English, preserving technical terms, removing code examples, and stripping the Chinese-only column from the spec-coverage table.
---

# Test Plan Translation (Chinese → English)

Translate the given Chinese markdown test plan file into English and save the result.

## Arguments

The user provides one or more file paths (Chinese markdown test plans) as arguments.
Example usage: `/testplan_translation DOCS/testplan/svnapot_test_plan.md`

## Instructions

For each input file, perform the following steps **in order**:

### Step 1: Read the input file and determine the output filename

1. Read the entire source file content. If the file does not exist, report the error and stop.
2. Determine the output filename: strip the file extension, append `_en`, re-add the extension.
   - `svnapot_test_plan.md` → `svnapot_test_plan_en.md`
   - `svade_test_plan.md` → `svade_test_plan_en.md`
3. Check whether the output file already exists in the same directory.

### Step 2: Choose translation mode

- **If the output file does NOT exist** → full translation mode. Proceed to Step 3.
- **If the output file already exists** → incremental update mode:
  1. Read the existing English file.
  2. Compare the source Chinese file against the existing English file **section by section** (split on `##`/`###`/`####` headers and table blocks).
  3. Identify which sections have been added, removed, or modified in the source.
     - A section is "modified" if its Chinese content no longer matches the corresponding English translation (accounting for the fact that code blocks and the 中文说明 column are stripped).
     - Norm ID rows in the spec-coverage table can be compared row-by-row by Norm ID key.
     - Test table rows can be compared by Test ID key.
  4. **If no changes are detected** → report "up to date, no changes needed" and stop processing this file.
  5. **If changes are detected** → for each changed section, apply the same transformation rules (Steps 3–5 below) only to that section, then merge the updated sections back into the existing English file. Preserve all unchanged sections exactly as they are.

### Step 3: Remove all code example blocks

Delete every fenced code block that uses the ```` ```c ```` (or ```` ```C ````) language tag, including the opening and closing fences and all content between them. These are implementation examples that should not appear in the translated document.

Also remove any blank lines left behind by the deletion (collapse consecutive blank lines into at most one).

### Step 4: Process the "覆盖的规范点" table

Locate the `### 覆盖的规范点` section. Its table has three columns:

| Norm ID | 原文 | 中文说明 |

- **Remove the `中文说明` column entirely** (both header and all data cells).
- **Keep only the `Norm ID` and `原文` columns.**
- The `原文` column already contains the original English spec text — do NOT translate or modify it.
- Rename the section header from `### 覆盖的规范点` to `### Covered Specification Points`.

### Step 5: Translate the rest of the document

Translate all remaining Chinese text to English, following these rules:

1. **Do NOT translate:**
   - Technical terms and abbreviations already in English (e.g., PTE, NAPOT, page-fault, PPN, VPN, RWX, A/D bit, SFENCE.VMA, satp, etc.)
   - RISC-V register names, CSR names, instruction names
   - Code identifiers, function names, variable names, macro names (e.g., `PTE_V`, `NAPOT_64K`, `test_va`)
   - Norm IDs (e.g., `norm:Svnapot_pte_N`)
   - File paths and references (e.g., `SPEC/svnapot.adoc`)
   - Test IDs (e.g., `NAPOT64-01`, `PPN-01`)
   - Hex values, addresses, bit patterns

2. **Translate:**
   - Chinese section headers (e.g., `测试目标` → `Test Objectives`, `测试分组` → `Test Groups`, `规范依据` → `Spec Reference`, `测试职责` → `Test Scope`)
   - Chinese table headers (e.g., `测试 ID` → `Test ID`, `测试名称` → `Test Name`, `测试描述` → `Test Description`, `预期结果` → `Expected Result`)
   - Chinese descriptions in table cells
   - Chinese narrative text and bullet points
   - Chinese comments or notes

3. **Style:**
   - Use concise, technical English appropriate for a test plan document
   - Match the tone and terminology conventions of RISC-V specification documents
   - Keep markdown formatting intact (headers, tables, lists, bold, inline code, horizontal rules)

### Step 6: Write the output file

Write the translated content to the output file. Then report:

- **Full translation mode:**
  - Input file path
  - Output file path
  - Number of code blocks removed
  - Confirmation that the 中文说明 column was stripped

- **Incremental update mode:**
  - Input file path and output file path
  - Which sections were updated (added / modified / removed)
  - Number of code blocks removed from changed sections
  - Sections left unchanged

- Any issues encountered during translation
