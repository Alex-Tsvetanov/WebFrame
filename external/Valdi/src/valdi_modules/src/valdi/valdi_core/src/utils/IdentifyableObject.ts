export interface IdentifyableObject {
  $objectId: number;
}

let idSequence = 0;

export function setObjectId(object: any) {
  if (!object) {
    return;
  }

  (object as IdentifyableObject).$objectId = ++idSequence;
}

export function getObjectId(object: any): number | undefined {
  if (!object) {
    return undefined;
  }

  return (object as IdentifyableObject).$objectId;
}
