import { encodeUtf8, ObjectReference, sendIpcMessage } from "skyrimPlatform";
import { Movement } from "./movement";

const MOVEMENT_APPLY_REQUEST_SIZE = 78;

// Corresponds to the C++ IpcStructs::MovementData
interface MovementData {
    worldOrCell: number;
    pos: [number, number, number];
    rot: [number, number, number];
    direction: number;
    healthPercentage: number;
    speed: number;
    runMode: string; // This will be encoded into a 16-byte char array
    isInJumpState: boolean;
    isSneaking: boolean;
    isBlocking: boolean;
    isWeapDrawn: boolean;
    isDead: boolean;
    lookAt: [number, number, number]; // Use [0, 0, 0] for nullopt
}

// Corresponds to the C++ IpcStructs::MovementApplyRequest
interface MovementApplyRequest {
    formId: number;
    isMyClone: boolean;
    movementData: MovementData;
}

/**
* Encodes and sends a movement update to the C++ plugin.
* @param request The movement data to send.
*/
function sendMovementApply(request: MovementApplyRequest): void {
    const buffer = encodeMovementApplyRequest(request);
    sendIpcMessage("MovementPlugin_MovementApply", buffer);
}

/**
 * Encodes a MovementApplyRequest object into an ArrayBuffer that matches the
 * C++ struct layout.
 * @param request The request object to encode.
 * @returns An ArrayBuffer containing the serialized data.
 */
function encodeMovementApplyRequest(request: MovementApplyRequest): ArrayBuffer {
    // Create a buffer with a fixed size matching the C++ struct
    const buffer = new ArrayBuffer(MOVEMENT_APPLY_REQUEST_SIZE);
    const view = new DataView(buffer);
    const bufferArray = new Uint8Array(buffer);
    let offset = 0;

    // --- Serialize MovementApplyRequest ---
    view.setUint32(offset, request.formId, true);
    offset += 4;

    view.setUint8(offset, request.isMyClone ? 1 : 0);
    offset += 1;

    // --- Serialize nested MovementData ---
    const data = request.movementData;

    view.setUint32(offset, data.worldOrCell, true);
    offset += 4;

    data.pos.forEach(p => {
        view.setFloat32(offset, p, true);
        offset += 4;
    });

    data.rot.forEach(r => {
        view.setFloat32(offset, r, true);
        offset += 4;
    });

    view.setFloat32(offset, data.direction, true);
    offset += 4;

    view.setFloat32(offset, data.healthPercentage, true);
    offset += 4;

    view.setFloat32(offset, data.speed, true);
    offset += 4;

    // Encode runMode string into a 16-byte buffer
    const runModeBytes = new Uint8Array(16);
    const encodedString = encodeUtf8(data.runMode);
    runModeBytes.set(new Uint8Array(encodedString).slice(0, 15)); // Ensure null termination
    bufferArray.set(runModeBytes, offset);
    offset += 16;

    // Booleans are serialized as single bytes (0 or 1)
    view.setUint8(offset++, data.isInJumpState ? 1 : 0);
    view.setUint8(offset++, data.isSneaking ? 1 : 0);
    view.setUint8(offset++, data.isBlocking ? 1 : 0);
    view.setUint8(offset++, data.isWeapDrawn ? 1 : 0);
    view.setUint8(offset++, data.isDead ? 1 : 0);

    data.lookAt.forEach(l => {
        view.setFloat32(offset, l, true);
        offset += 4;
    });

    return buffer;
}

function convertMovementData(movement: Movement): MovementData {
    return {
        worldOrCell: movement.worldOrCell,
        pos: movement.pos,
        rot: movement.rot,
        direction: movement.direction,
        healthPercentage: movement.healthPercentage,
        speed: movement.speed,
        runMode: movement.runMode,
        isInJumpState: movement.isInJumpState,
        isSneaking: movement.isSneaking,
        isBlocking: movement.isBlocking,
        isWeapDrawn: movement.isWeapDrawn,
        isDead: movement.isDead,
        lookAt: movement.lookAt || [0, 0, 0],
    };
}

export const applyMovementNg = (refr: ObjectReference, m: Movement, isMyClone?: boolean): void => {
    const movementData = convertMovementData(m);
    sendMovementApply({
        formId: refr.getFormID(),
        isMyClone: isMyClone === true,
        movementData,
    });
}
