import { test } from 'node:test';
import assert from 'node:assert/strict';
import { execFileSync, spawnSync } from 'node:child_process';
import { copyFileSync, mkdirSync, mkdtempSync, rmSync, writeFileSync } from 'node:fs';
import { join, resolve } from 'node:path';
import { documentationCriteria } from './review-docs.mjs';

test('real adapter rejects below-threshold PASS and accepts the configured boundary', () => {
  const workspace = resolve('.local');
  mkdirSync(workspace, { recursive: true });
  const repository = mkdtempSync(join(workspace, 'review-threshold-'));
  const fixtureEnvironment = { ...process.env };
  const repositoryVariables = execFileSync('git', ['rev-parse', '--local-env-vars'], { encoding: 'utf8' }).trim().split('\n');
  for (const variableName of repositoryVariables) {
    delete fixtureEnvironment[variableName.trim()];
  }
  const git = (...argumentsList) => execFileSync('git', argumentsList, {
    cwd: repository, encoding: 'utf8', env: fixtureEnvironment,
  }).trim();
  try {
    git('init', '--initial-branch=main');
    git('config', 'user.name', 'Review Fixture');
    git('config', 'user.email', 'review@example.invalid');
    git('config', 'commit.gpgsign', 'false');
    git('config', 'core.hooksPath', join(repository, 'disabled-hooks'));
    writeFileSync(join(repository, '.gitignore'), '.local/\n');
    writeFileSync(join(repository, 'value.txt'), 'before\n');
    git('add', '.');
    git('commit', '-m', 'fixture baseline');
    const base = git('rev-parse', 'HEAD');
    mkdirSync(join(repository, '.claude'));
    mkdirSync(join(repository, 'scripts'));
    mkdirSync(join(repository, '.local'));
    for (const filename of ['review-check.mjs', 'review-docs.mjs']) {
      copyFileSync(join('scripts', filename), join(repository, 'scripts', filename));
    }
    writeFileSync(join(repository, '.claude/review.config.json'), JSON.stringify({
      base,
      persona: 'plain',
      secretAllowlist: [],
      agents: [{ name: 'craft', threshold: 8 }, { name: 'docs', threshold: 8 }],
      gates: [],
    }));
    writeFileSync(join(repository, 'value.txt'), 'after\n');
    git('add', '.');
    git('commit', '-m', 'fixture change');
    const resultsPath = join(repository, '.local/results.json');
    const results = {
      craft: { score: 7, verdict: 'PASS' },
      architecture: { score: 10, verdict: 'PASS' },
      tests: { score: 10, verdict: 'PASS' },
      security: { score: 10, verdict: 'PASS' },
      docs: {
        score: 10,
        verdict: 'PASS',
        review: {
          reviewer: 'Synthetic adapter fixture only',
          files: [],
          criteria: Object.fromEntries(documentationCriteria.map((criterion) => [
            criterion, { verdict: 'PASS', evidence: 'Synthetic test data, no publication approval.' },
          ])),
          findings: [],
        },
      },
    };
    writeFileSync(resultsPath, JSON.stringify(results));
    const rejected = spawnSync(process.execPath, ['scripts/review-check.mjs', '--attest', resultsPath],
      { cwd: repository, encoding: 'utf8', env: fixtureEnvironment });
    assert.notEqual(rejected.status, 0);
    assert.match(rejected.stderr, /insufficient review result for craft/);
    results.craft.score = 8;
    writeFileSync(resultsPath, JSON.stringify(results));
    const accepted = spawnSync(process.execPath, ['scripts/review-check.mjs', '--attest', resultsPath],
      { cwd: repository, encoding: 'utf8', env: fixtureEnvironment });
    assert.equal(accepted.status, 0, accepted.stdout + accepted.stderr);
    assert.match(accepted.stdout, /Review passed/);
  } finally {
    assert.ok(repository.startsWith(workspace + '\\') || repository.startsWith(workspace + '/'));
    rmSync(repository, { recursive: true, force: true });
  }
});
