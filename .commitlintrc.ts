import {
	RuleConfigCondition,
	RuleConfigSeverity,
	TargetCaseType,
} from '@commitlint/types';

export default {
	parserPreset: 'conventional-changelog-conventionalcommits',
	rules: {
		'body-leading-blank': [RuleConfigSeverity.Warning, 'always'] as const,
		'body-max-line-length': [RuleConfigSeverity.Error, 'always', 70] as const,
		'footer-leading-blank': [RuleConfigSeverity.Warning, 'always'] as const,
		'footer-max-line-length': [
			RuleConfigSeverity.Error,
			'always',
			70,
		] as const,
		'header-max-length': [RuleConfigSeverity.Error, 'always', 70] as const,
		'header-trim': [RuleConfigSeverity.Error, 'always'] as const,
		'subject-case': [
			RuleConfigSeverity.Error,
			'never',
			['sentence-case', 'start-case', 'pascal-case', 'upper-case'],
		] as [RuleConfigSeverity, RuleConfigCondition, TargetCaseType[]],
		'subject-empty': [RuleConfigSeverity.Error, 'never'] as const,
		'subject-full-stop': [RuleConfigSeverity.Error, 'never', '.'] as const,
		'type-case': [RuleConfigSeverity.Error, 'always', 'lower-case'] as const,
		'type-empty': [RuleConfigSeverity.Error, 'never'] as const,
		'type-enum': [
			RuleConfigSeverity.Error,
			'always',
			[
				'build',
				'chore',
				'ci',
				'docs',
				'feat',
				'fix',
				'perf',
				'refactor',
				'revert',
				'style',
				'test',
			],
		] as [RuleConfigSeverity, RuleConfigCondition, string[]],
	}
};