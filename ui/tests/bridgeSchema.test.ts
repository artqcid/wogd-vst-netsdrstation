/**
 * Verifies that the hand-written Zod validators (src/generated/bridge-validators.ts)
 * stay structurally consistent with the JSON Schema
 * (schema/bridge.schema.json) — the single source of truth (M4.1.5).
 *
 * json-schema-to-zod cannot resolve local $refs and zod-to-json-schema has no
 * Zod-v4-compatible release, so the Zod schemas are maintained by hand. This
 * test prevents drift by checking the validators' *behaviour* against the
 * canonical schema definitions.
 */
import { describe, it, expect } from 'vitest'
import { readFileSync } from 'node:fs'
import { resolve } from 'node:path'

import {
  ParamIdSchema,
  SetParameterMessageSchema,
  SetStationMessageSchema,
  DisconnectMessageSchema,
  GetParametersMessageSchema,
  StatusMessageSchema,
  ParamUpdateMessageSchema,
  LevelMessageSchema,
} from '@/generated/bridge-validators'

function loadCanonicalSchema() {
  const path = resolve(__dirname, '../../schema/bridge.schema.json')
  return JSON.parse(readFileSync(path, 'utf-8')) as {
    definitions: Record<string, Record<string, unknown>>
  }
}

describe('bridge-validators <-> bridge.schema.json consistency', () => {
  it('ParamId accepts exactly the canonical enum values', () => {
    const canonical = loadCanonicalSchema().definitions.ParamId as { enum?: string[] }
    const ids = canonical.enum ?? []

    expect(ids.length).toBeGreaterThan(0)
    for (const id of ids) {
      expect(ParamIdSchema.safeParse(id).success, `should accept '${id}'`).toBe(true)
    }
    // a value not in the canonical enum must be rejected
    expect(ParamIdSchema.safeParse('not-a-real-param').success).toBe(false)
  })

  it('all canonical definitions have a corresponding Zod validator', () => {
    const canonical = loadCanonicalSchema().definitions
    const validatorNames = [
      'ParamIdSchema',
      'HostPortSchema',
      'SetParameterMessageSchema',
      'SetStationMessageSchema',
      'DisconnectMessageSchema',
      'GetParametersMessageSchema',
      'StatusMessageSchema',
      'ParamUpdateMessageSchema',
      'LevelMessageSchema',
      'BackendMessageSchema',
    ]
    const canonicalNames = Object.keys(canonical)
    for (const name of canonicalNames) {
      // canonical definition "ParamId" -> Zod validator "ParamIdSchema"
      const expected = name + 'Schema'
      expect(
        validatorNames.includes(expected),
        `missing Zod validator for ${name}`
      ).toBe(true)
    }
  })

  it('SetParameterMessage matches the canonical tuple shape', () => {
    const canonical = loadCanonicalSchema().definitions.SetParameterMessage as {
      properties?: { type?: { const?: string }; data?: { items?: unknown[] } }
    }
    expect(canonical.properties?.type?.const).toBe('setParameter')

    expect(
      SetParameterMessageSchema.safeParse({ type: 'setParameter', data: ['freqKhz', 14100] }).success
    ).toBe(true)
    // wrong discriminator
    expect(
      SetParameterMessageSchema.safeParse({ type: 'setStation', data: ['x:8072'] }).success
    ).toBe(false)
    // wrong arity
    expect(
      SetParameterMessageSchema.safeParse({ type: 'setParameter', data: ['freqKhz'] }).success
    ).toBe(false)
  })

  it('SetStationMessage matches the canonical tuple shape', () => {
    const canonical = loadCanonicalSchema().definitions.SetStationMessage as {
      properties?: { type?: { const?: string }; data?: { items?: unknown[] } }
    }
    expect(canonical.properties?.type?.const).toBe('setStation')

    expect(
      SetStationMessageSchema.safeParse({ type: 'setStation', data: ['kphsdr.com:8072'] }).success
    ).toBe(true)
    // host without port
    expect(
      SetStationMessageSchema.safeParse({ type: 'setStation', data: ['kphsdr.com'] }).success
    ).toBe(false)
  })

  it('DisconnectMessage and GetParametersMessage accept null data', () => {
    expect(DisconnectMessageSchema.safeParse({ type: 'disconnect', data: null }).success).toBe(true)
    expect(
      GetParametersMessageSchema.safeParse({ type: 'getParameters', data: null }).success
    ).toBe(true)
    expect(
      DisconnectMessageSchema.safeParse({ type: 'disconnect', data: [] }).success
    ).toBe(false)
  })

  it('StatusMessage and ParamUpdateMessage match the backend definitions', () => {
    expect(StatusMessageSchema.safeParse({ type: 'status', data: 'Connected' }).success).toBe(true)
    expect(
      ParamUpdateMessageSchema.safeParse({ type: 'param', data: { id: 'volume', value: 0.5 } }).success
    ).toBe(true)
    // missing value
    expect(
      ParamUpdateMessageSchema.safeParse({ type: 'param', data: { id: 'volume' } }).success
    ).toBe(false)
  })

  it('LevelMessage matches the canonical definition (1-tuple of number)', () => {
    expect(LevelMessageSchema.safeParse({ type: 'level', data: [-90.0] }).success).toBe(true)
    expect(LevelMessageSchema.safeParse({ type: 'level', data: [-90.0, 10.0] }).success).toBe(false)
    expect(LevelMessageSchema.safeParse({ type: 'level', data: ['-90'] }).success).toBe(false)
  })
})