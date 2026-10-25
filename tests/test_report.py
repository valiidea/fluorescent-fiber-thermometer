"""Check metric definitions and prevent incomplete benchmark reports."""
import copy
import csv
import importlib.util
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

BINARY = str(Path(sys.argv.pop(1)).resolve())
SPEC = importlib.util.spec_from_file_location('report', Path(__file__).resolve().parents[1] / 'tools/report.py')
report = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(report)


class Report(unittest.TestCase):
    def test_metric_definitions(self):
        stats = report.summarize([{'status': 'ok', 'error_percent': '-10'},
                                  {'status': 'ok', 'error_percent': '10'},
                                  {'status': 'not_decaying', 'error_percent': ''}])
        self.assertEqual(stats['rmse_percent'], 10)
        self.assertEqual(stats['bias_percent'], 0)
        self.assertEqual(stats['p95_abs_percent'], 10)
        self.assertAlmostEqual(stats['rejected_percent'], 100/3)
        self.assertIsNone(report.summarize([{'status': 'not_decaying'}])['rmse_percent'])

    def test_integrity(self):
        result = subprocess.run([BINARY, 'benchmark', '--trials', '2'], text=True, capture_output=True, check=True)
        with tempfile.TemporaryDirectory() as folder:
            path = Path(folder) / 'benchmark.csv'
            path.write_text(result.stdout)
            records, pairs, trials, seed = report.load(path)
            self.assertEqual((len(records), len(pairs), trials, seed), (240, 80, 2, '42'))
            report.generate(path, Path(folder) / 'report')
            self.assertTrue((Path(folder) / 'report/results.md').exists())
            damaged = copy.deepcopy(records)
            damaged[0]['error_percent'] = '999'
            for bad in [records[:-1], records + [records[0]], damaged,
                        [r for r in records if r['trial'] == '1']]:
                with path.open('w', newline='') as stream:
                    writer = csv.DictWriter(stream, fieldnames=records[0])
                    writer.writeheader()
                    writer.writerows(bad)
                with self.assertRaises(ValueError):
                    report.load(path)


if __name__ == '__main__':
    unittest.main()
