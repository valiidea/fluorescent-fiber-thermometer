"""End-to-end contracts and an independent dense-grid NLS reference; stdlib only."""
import csv
import io
import math
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

BINARY = str(Path(sys.argv.pop(1)).resolve())


def run(*args, data=None):
    return subprocess.run([BINARY, *args], input=data, text=True, capture_output=True)


def rows(text):
    return list(csv.DictReader(io.StringIO(text)))


class CLI(unittest.TestCase):
    def setUp(self):
        self.sample = run('simulate', '--frames', '4', '--noise', '0').stdout

    def analyze(self, text, *args):
        return run('analyze', '--input', '-', *args, data=text)

    def test_help_and_options(self):
        self.assertEqual(run('--help').returncode, 0)
        for args in [(), ('unknown',), ('simulate', '--frames', '-1'),
                     ('simulate', '--frames', '1.5'), ('simulate', '--noise', 'nan'),
                     ('simulate', '--tau', '0'), ('simulate', '--frames', '0'),
                     ('simulate', '--seed', '4294967296'), ('analyze',),
                     ('analyze', '--input', '-', '--alpha', '0'),
                     ('simulate', '--tau'), ('benchmark', '--trials', '10001')]:
            with self.subTest(args=args):
                self.assertEqual(run(*args).returncode, 2)

    def test_deterministic_roundtrip(self):
        a = run('simulate', '--seed', '99', '--frames', '3')
        b = run('simulate', '--seed', '99', '--frames', '3')
        self.assertEqual(a.returncode, 0)
        self.assertEqual(a.stdout, b.stdout)
        for method in ['window', 'log', 'nls']:
            output = self.analyze(self.sample, '--method', method)
            self.assertEqual(output.returncode, 0, output.stderr)
            self.assertEqual(len(rows(output.stdout)), 4)
            for row in rows(output.stdout):
                self.assertEqual(row['status'], 'ok')
                self.assertAlmostEqual(float(row['tau']), .3, places=6)

    def test_csv_errors(self):
        header, line, *_ = self.sample.splitlines()
        variants = ['', header + '\n', 'bad\n' + line,
                    header + '\n' + line + ',extra\n',
                    header + '\n' + ','.join(line.split(',')[:-1]),
                    header + '\n' + ','.join([line.split(',')[0], 'nan', *line.split(',')[2:]]),
                    header + '\n' + '9' * 5000,
                    header + '\n' + line + '\0,extra\n',
                    header + '\n' + line + '\n' + line + '\n']
        for data in variants:
            with self.subTest(data=data[:40]):
                self.assertEqual(self.analyze(data).returncode, 1)
        self.assertEqual(self.analyze(self.sample.replace('\n', '\r\n')).returncode, 0)
        self.assertEqual(self.analyze(self.sample.rstrip('\n')).returncode, 0)
        self.assertEqual(run('analyze', '--input', '/nonexistent/frames.csv').returncode, 1)

    def test_file_input_and_configuration(self):
        with tempfile.TemporaryDirectory() as folder:
            path = Path(folder) / 'input.csv'
            path.write_text(self.sample)
            out = run('analyze', '--input', str(path), '--early', '0', '--late', '31', '--width', '1')
            self.assertEqual(out.returncode, 0, out.stderr)
        for args in [('--width', '0'), ('--late', '33'), ('--method', 'unknown'),
                     ('--tau-min', '3'), ('--tau-max', 'nan')]:
            self.assertEqual(self.analyze(self.sample, *args).returncode, 2)

    def test_invalid_frame_resets_filter(self):
        first = rows(run('simulate', '--frames', '1', '--tau', '.3', '--noise', '0').stdout)[0]
        second = dict(first)
        second['frame'] = '1'
        for key in second:
            if key.startswith('s'):
                second[key] = second['background']
        third = rows(run('simulate', '--frames', '1', '--tau', '.6', '--noise', '0').stdout)[0]
        third['frame'] = '2'
        stream = io.StringIO()
        writer = csv.DictWriter(stream, fieldnames=first)
        writer.writeheader()
        writer.writerows([first, second, third])
        out = self.analyze(stream.getvalue())
        self.assertEqual(out.returncode, 0, out.stderr)
        result = rows(out.stdout)
        self.assertEqual(result[1]['status'], 'nonpositive_signal')
        self.assertEqual(result[1]['tau'], '')
        self.assertEqual(result[1]['smoothed'], '')
        self.assertAlmostEqual(float(result[2]['smoothed']), .6, places=6)
        self.assertEqual(self.analyze(stream.getvalue(), '--tau-max', '.1').returncode, 3)

    def test_dense_grid_reference(self):
        sample = run('simulate', '--frames', '1', '--seed', '71', '--noise', '.03').stdout
        frame = rows(sample)[0]
        data = [float(frame[f's{i}']) - float(frame['background']) for i in range(4, 20)]

        def sse(tau):
            g = [math.exp(-i / 31 / tau) for i in range(4, 20)]
            amplitude = sum(y * x for y, x in zip(data, g)) / sum(x*x for x in g)
            return sum((y - amplitude*x)**2 for y, x in zip(data, g))

        # Independent exhaustive scan, without the C solver's profiled local coordinates.
        candidates = [math.exp(math.log(.03) + i / 20000 * math.log(2/.03)) for i in range(20001)]
        reference = min(candidates, key=sse)
        result = rows(self.analyze(sample).stdout)[0]
        estimate = float(result['tau'])
        self.assertLessEqual(sse(estimate), sse(reference) + 1e-12)
        self.assertLess(abs(estimate/reference - 1), .001)
        self.assertAlmostEqual(float(result['rmse']), math.sqrt(sse(estimate)/16), places=12)

    def test_benchmark_schema_and_failure_accounting(self):
        out = run('benchmark', '--trials', '2')
        self.assertEqual(out.returncode, 0, out.stderr)
        records = rows(out.stdout)
        self.assertEqual(len(records), 240)
        self.assertEqual({r['method'] for r in records}, {'window', 'log', 'nls'})
        self.assertEqual(len({r['scenario'] for r in records}), 5)
        for row in records:
            if row['status'] == 'ok':
                self.assertTrue(math.isfinite(float(row['tau'])))
            else:
                self.assertEqual(row['tau'], '')


if __name__ == '__main__':
    unittest.main()
