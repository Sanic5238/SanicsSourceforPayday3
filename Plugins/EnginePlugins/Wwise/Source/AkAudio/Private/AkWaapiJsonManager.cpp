#include "AkWaapiJsonManager.h"

UAkWaapiJsonManager::UAkWaapiJsonManager() {
}

FAKWaapiJsonObject UAkWaapiJsonManager::SetStringField(const FAkWaapiFieldNames& FieldName, const FString& FieldValue, FAKWaapiJsonObject target) {
    return FAKWaapiJsonObject{};
}

FAKWaapiJsonObject UAkWaapiJsonManager::SetObjectField(const FAkWaapiFieldNames& FieldName, FAKWaapiJsonObject FieldValue, FAKWaapiJsonObject target) {
    return FAKWaapiJsonObject{};
}

FAKWaapiJsonObject UAkWaapiJsonManager::SetNumberField(const FAkWaapiFieldNames& FieldName, float FieldValue, FAKWaapiJsonObject target) {
    return FAKWaapiJsonObject{};
}

FAKWaapiJsonObject UAkWaapiJsonManager::SetBoolField(const FAkWaapiFieldNames& FieldName, bool FieldValue, FAKWaapiJsonObject target) {
    return FAKWaapiJsonObject{};
}

FAKWaapiJsonObject UAkWaapiJsonManager::SetArrayStringFields(const FAkWaapiFieldNames& FieldName, const TArray<FString>& FieldStringValues, FAKWaapiJsonObject target) {
    return FAKWaapiJsonObject{};
}

FAKWaapiJsonObject UAkWaapiJsonManager::SetArrayObjectFields(const FAkWaapiFieldNames& FieldName, const TArray<FAKWaapiJsonObject>& FieldObjectValues, FAKWaapiJsonObject target) {
    return FAKWaapiJsonObject{};
}

FString UAkWaapiJsonManager::GetStringField(const FAkWaapiFieldNames& FieldName, FAKWaapiJsonObject target) {
    return TEXT("");
}

FAKWaapiJsonObject UAkWaapiJsonManager::GetObjectField(const FAkWaapiFieldNames& FieldName, FAKWaapiJsonObject target) {
    return FAKWaapiJsonObject{};
}

float UAkWaapiJsonManager::GetNumberField(const FAkWaapiFieldNames& FieldName, FAKWaapiJsonObject target) {
    return 0.0f;
}

int32 UAkWaapiJsonManager::GetIntegerField(const FAkWaapiFieldNames& FieldName, FAKWaapiJsonObject target) {
    return 0;
}

bool UAkWaapiJsonManager::GetBoolField(const FAkWaapiFieldNames& FieldName, FAKWaapiJsonObject target) {
    return false;
}

TArray<FAKWaapiJsonObject> UAkWaapiJsonManager::GetArrayField(const FAkWaapiFieldNames& FieldName, FAKWaapiJsonObject target) {
    return TArray<FAKWaapiJsonObject>();
}

FText UAkWaapiJsonManager::Conv_FAKWaapiJsonObjectToText(FAKWaapiJsonObject INAKWaapiJsonObject) {
    return FText::GetEmpty();
}

FString UAkWaapiJsonManager::Conv_FAKWaapiJsonObjectToString(FAKWaapiJsonObject INAKWaapiJsonObject) {
    return TEXT("");
}


