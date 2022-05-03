export class Form {
  constructor(formId: number) {}

  GetFormId(): number;
  GetName(): string;
  GetGoldValue(): number;
  GetWeight(): number;
  GetKeywords(): number[];
  GetNthKeyword(): number;
  GetNumKeywords(): number;
  HasKeyword(keyword: number): boolean;
  GetType(): string;
  GetEditorId(): string;
  GetSignature(): string;
  EqualSignature(signature: string): boolean;
  GetDescription(): string;
}
