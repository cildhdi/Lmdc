// by cildhdi

#pragma once

#include <qjsonarray.h>
#include <qjsondocument.h>
#include <qjsonobject.h>
#include <map>

namespace mxt {

struct Vector {
  double x = 0.0, y = 0.0, z = 0.0;
  Vector() {}
  Vector(QJsonArray const& array) {
    if (array.size() == 3) {
      if (array.at(0).isDouble()) x = array.at(0).toDouble();
      if (array.at(1).isDouble()) y = array.at(1).toDouble();
      if (array.at(2).isDouble()) z = array.at(2).toDouble();
    }
  }
};
// https://github.com/leapmotion/leapjs/blob/31b00723f98077304acda3200f9fbcbaaf29294a/lib/finger.js#L136
enum BoneType { Metacarpal, Proximal, Medial, Distal };
struct Bone {
  Vector prevJoint, nextJoint;
};

enum FingerType { Thumb, Index, Middle, Ring, Pinky };
using Finger = std::map<BoneType, Bone>;

enum class HandType { Left, Right };
struct Hand {
  int id;
  HandType type;
  std::map<FingerType, Finger> fingers;
};

struct Frame {
  long long id = 0;
  long long timestamp = 0;
  std::map<int, Hand> hands;

  auto findHandByType(HandType type) {
    auto it = hands.begin();
    while (it != hands.end() && it->second.type != type) ++it;
    return it;
  }

  Frame(QJsonObject const& json) {
    if (json.contains("id")) {
      QJsonValue value = json.value("id");
      if (value.isDouble()) id = value.toVariant().toLongLong();
    }
    if (json.contains("timestamp")) {
      QJsonValue value = json.value("timestamp");
      if (value.isDouble()) timestamp = value.toVariant().toLongLong();
    }
    if (json.contains("hands")) {
      QJsonValue handsJson = json.value("hands");
      if (handsJson.isArray()) {
        QJsonArray handsArray = handsJson.toArray();
        for (auto& handJson : handsArray) {
          auto handObject = handJson.toObject();
          Hand hand;
          if (handObject.contains("id"))
            hand.id = handObject.value("id").toInt();
          if (handObject.contains("type"))
            hand.type = handObject.value("type").toString() == "left"
                            ? HandType::Left
                            : HandType::Right;
          hands[hand.id] = hand;
        }
      }
    }
    if (json.contains("pointables")) {
      auto pointables = json.value("pointables").toArray();
      for (auto& pointableJson : pointables) {
        auto pointableObject = pointableJson.toObject();
        int handId = pointableObject.value("handId").toInt();
        if (hands.find(handId) != hands.end()) {
          Finger finger;
          Bone bone;

#define BONES_TO_FINGER(prev, next, boneType)                      \
  bone.prevJoint = Vector(pointableObject.value(#prev).toArray()); \
  bone.nextJoint = Vector(pointableObject.value(#next).toArray()); \
  finger[boneType] = bone;

          BONES_TO_FINGER(carpPosition, mcpPosition, BoneType::Metacarpal)
          BONES_TO_FINGER(mcpPosition, pipPosition, BoneType::Proximal)
          BONES_TO_FINGER(pipPosition, dipPosition, BoneType::Medial)
          BONES_TO_FINGER(dipPosition, btipPosition, BoneType::Distal)

#undef BONES_TO_FINGER

          FingerType fingerType =
              static_cast<FingerType>(pointableObject.value("type").toInt());

          hands[handId].fingers[fingerType] = finger;
        }
      }
    }
  }
};

}  // namespace mxt