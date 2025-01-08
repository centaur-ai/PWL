import json
import re
import sys
from datetime import datetime, timezone
import uuid

def parse_section(text):
    sections = re.split(r'(Successfully added logical form as an observation to the theory\.|Finished reading declarative sentences\. Attempting to answer question:)', text)
    results = []

    line_id_map = {}

    current_section = ""
    for i, section in enumerate(sections):
        if section.strip() in [
            "Successfully added logical form as an observation to the theory.",
            "Finished reading declarative sentences. Attempting to answer question:"
        ]:
            if i + 1 < len(sections):
                current_section = section + sections[i + 1]

                lines = current_section.strip().split('\n')
                theory_log_prob = None

                for line in lines:
                    if "Theory log probability:" in line:
                        theory_log_prob = line.split("Theory log probability:")[1].strip()
                        break

                for line_idx, line in enumerate(lines):
                    if "Logical form:" in line:
                        logical_form_id = str(uuid.uuid4())
                        logical_form = line.split("Logical form:")[1].strip()
                        logical_form_obj = {
                            "id": logical_form_id,
                            "type": "logical_form",
                            "axiom": logical_form,
                            "description": section.strip(),
                            "theory_log_probability": theory_log_prob,
                            "created_at": datetime.now(timezone.utc).isoformat().replace('+00:00', 'Z')
                        }
                        results.append(logical_form_obj)

                        line_id_map[line_idx] = logical_form_id

                        proof_start = current_section.find('Proof of newly-added logical form')
                        if proof_start != -1:
                            proof_section = current_section[proof_start:]
                            proof_id = str(uuid.uuid4())
                            steps = []

                            proof_lines = proof_section.split('\n')
                            for proof_line_idx, proof_line in enumerate(proof_lines):
                                if proof_line.strip() and '[' in proof_line:
                                    match = re.match(r'\s*\[(\d+)\]\s*(.*?)\s*by\s*(.*?)$', proof_line.strip())
                                    if match:
                                        step_id = str(uuid.uuid4())
                                        step_num = int(match.group(1))
                                        formula = match.group(2)
                                        justification = "by " + match.group(3)
                                        theory_objects = sorted(set(re.findall(r'c[₀₁₂₃₄₅₆₇₈₉]+', formula)))
                                        steps.append({
                                            "id": step_id,
                                            "step": step_num,
                                            "formula": formula,
                                            "justification": justification,
                                            "theory_objects": theory_objects,
                                        })

                                        line_id_map[line_idx+1+proof_line_idx] = step_id

                            if steps:
                                proof_obj = {
                                    "id": proof_id,
                                    "type": "proof",
                                    "steps": steps,
                                    "created_at": datetime.now(timezone.utc).isoformat().replace('+00:00', 'Z')
                                }

                                for res in reversed(results):
                                    if res["type"] == "logical_form":
                                        res["proof_id"] = proof_id
                                        break

                                results.append(proof_obj)
                        break

    answer_match = re.search(r'Answer:\s*(\w+)\s*:\s*([\d.]+)\s*\w+\s*:\s*([\d.]+)', text)
    if answer_match:
        answer_id = str(uuid.uuid4())
        answer_obj = {
            "id": answer_id,
            "type": "answer",
            "result": answer_match.group(1),
            "true_rate": answer_match.group(2),
            "false_rate": answer_match.group(3),
            "created_at": datetime.now(timezone.utc).isoformat().replace('+00:00', 'Z')
        }
        results.append(answer_obj)

    stream_end_id = str(uuid.uuid4())
    results.append({
        "id": stream_end_id,
        "type": "stream_end",
        "message": "Reasoning session completed",
        "created_at": datetime.now(timezone.utc).isoformat().replace('+00:00', 'Z')
    })

    return results

def main():
    text_content = sys.stdin.read()

    results = parse_section(text_content)

    for result in results:
        print(json.dumps(result, ensure_ascii=False))

if __name__ == "__main__":
    main()
