#include "AkItemBoolPropertiesConv.h"

UAkItemBoolPropertiesConv::UAkItemBoolPropertiesConv() {
}

FText UAkItemBoolPropertiesConv::Conv_FAkBoolPropertyToControlToText(const FAkBoolPropertyToControl& INAkBoolPropertyToControl) {
    return FText::GetEmpty();
}

FString UAkItemBoolPropertiesConv::Conv_FAkBoolPropertyToControlToString(const FAkBoolPropertyToControl& INAkBoolPropertyToControl) {
    return TEXT("");
}


