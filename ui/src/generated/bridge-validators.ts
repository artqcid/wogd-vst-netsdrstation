/**
 * Runtime validators for the NetSDRStation bridge protocol.
 *
 * These Zod schemas mirror the JSON Schema contract in
 * `schema/bridge.schema.json` (the single source of truth). They are
 * maintained by hand (json-schema-to-zod cannot resolve local $refs) but
 * MUST stay structurally identical to the schema — `tests/bridgeSchema.test.ts`
 * verifies the correspondence via zod-to-json-schema.
 *
 * DO NOT widen a type here without updating the JSON Schema.
 */
import { z } from 'zod'

/** Stable UI-facing parameter name (see schema/bridge.schema.json ParamId). */
export const ParamIdSchema = z.enum([
  'mode',
  'freqKhz',
  'lowCut',
  'highCut',
  'agcOn',
  'agcHang',
  'agcThresh',
  'agcSlope',
  'agcDecay',
  'agcManGain',
  'volume',
  'mute',
  'squelchOn',
  'squelchThr',
  'nbOn',
  'nbThresh',
  'nrOn',
  'deempOn',
  'compOn',
  'wfOn',
  'wfSpeed',
  'wfZoom',
  'wfMaxDb',
  'wfMinDb',
  'wfComp',
  'arOn',
  'ovOn',
  'rfAttn',
  'cwPeaks',
])
export type ParamId = z.infer<typeof ParamIdSchema>

/** KiwiSDR station as 'host:port'. */
export const HostPortSchema = z.string().regex(/^[^:]+:[0-9]{1,5}$/)
export type HostPort = z.infer<typeof HostPortSchema>

/** Envelope object shared by all message types: {"type":T,"data":D}. */
const envelope = <T extends string, D extends z.ZodTypeAny>(type: T, data: D) =>
  z.object({ type: z.literal(type), data })

export const SetParameterMessageSchema = envelope(
  'setParameter',
  z.tuple([ParamIdSchema, z.number()])
)
export type SetParameterMessage = z.infer<typeof SetParameterMessageSchema>

export const SetStationMessageSchema = envelope('setStation', z.tuple([HostPortSchema]))
export type SetStationMessage = z.infer<typeof SetStationMessageSchema>

export const DisconnectMessageSchema = envelope('disconnect', z.null())
export type DisconnectMessage = z.infer<typeof DisconnectMessageSchema>

export const GetParametersMessageSchema = envelope('getParameters', z.null())
export type GetParametersMessage = z.infer<typeof GetParametersMessageSchema>

/** Messages sent UI -> C++ (dispatchable by the bridge). */
export const BridgeMessageSchema = z.discriminatedUnion('type', [
  SetParameterMessageSchema,
  SetStationMessageSchema,
  DisconnectMessageSchema,
  GetParametersMessageSchema,
])
export type BridgeMessage = z.infer<typeof BridgeMessageSchema>

/** Messages sent C++ -> UI (status text / parameter updates). */
export const StatusMessageSchema = envelope('status', z.string())
export type StatusMessage = z.infer<typeof StatusMessageSchema>

export const ParamUpdateMessageSchema = envelope(
  'param',
  z.object({ id: ParamIdSchema, value: z.number() })
)
export type ParamUpdateMessage = z.infer<typeof ParamUpdateMessageSchema>

/** Signal level in dBm (S-meter readout), sent C++ -> UI. */
export const LevelMessageSchema = envelope('level', z.tuple([z.number()]))
export type LevelMessage = z.infer<typeof LevelMessageSchema>

/** Spectrum bins in dBFS (-160..0), sent C++ -> UI (simulated spectrum, M4.7). */
export const WaterfallMessageSchema = envelope('waterfall', z.array(z.number()).min(2))
export type WaterfallMessage = z.infer<typeof WaterfallMessageSchema>

export const BackendMessageSchema = z.discriminatedUnion('type', [
  StatusMessageSchema,
  ParamUpdateMessageSchema,
  LevelMessageSchema,
  WaterfallMessageSchema,
])
export type BackendMessage = z.infer<typeof BackendMessageSchema>