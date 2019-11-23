// by cildhdi

#pragma once

#include <rapidjson/document.h>
#include <map>

#define LM_CASE(name) \
  case name:          \
    return #name;     \
    break;

namespace mxt {

struct Vector {
  double x = 0.0, y = 0.0, z = 0.0;
  Vector(double xv = 0.0, double yv = 0.0, double zv = 0.0)
      : x(xv), y(yv), z(zv) {}
  Vector(const rapidjson::GenericArray<false, rapidjson::Value>& array)
      : Vector() {
    if (array.Size() == 3) {
      if (array[0].IsDouble()) x = array[0].GetDouble();
      if (array[1].IsDouble()) y = array[1].GetDouble();
      if (array[2].IsDouble()) z = array[2].GetDouble();
    }
  }
};
// https://github.com/leapmotion/leapjs/blob/31b00723f98077304acda3200f9fbcbaaf29294a/lib/finger.js#L136

enum BoneType { Metacarpal, Proximal, Medial, Distal };
__forceinline const char* BoneTypeName(BoneType type) {
  switch (type) {
    LM_CASE(Metacarpal)
    LM_CASE(Proximal) LM_CASE(Medial) LM_CASE(Distal) default : return "";
    break;
  }
}

struct Bone {
  Vector prevJoint, nextJoint;

  Vector direction() const {
    return Vector((nextJoint.x - prevJoint.x), (nextJoint.y - prevJoint.y),
                  (nextJoint.z - prevJoint.z));
  }
};

enum FingerType { Thumb, Index, Middle, Ring, Pinky };
__forceinline const char* FingerTypeName(FingerType type) {
  switch (type) {
    LM_CASE(Thumb)
    LM_CASE(Index)
    LM_CASE(Middle) LM_CASE(Ring) LM_CASE(Pinky) default : return "";
    break;
  }
}
using Finger = std::map<BoneType, Bone>;

enum class HandType { Left, Right };
struct Hand {
  int id = 0;
  HandType type = HandType::Left;
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

  Frame(const char* text) {
    auto doc = rapidjson::Document();
    doc.Parse(text);
    const auto& json = doc.GetObject();
    if (auto idv = json.FindMember("id");
        idv != json.MemberEnd() && idv->value.IsInt64()) {
      id = idv->value.GetInt64();
    }
    if (auto tsv = json.FindMember("timestamp");
        tsv != json.MemberEnd() && tsv->value.IsInt64()) {
      timestamp = tsv->value.GetInt64();
    }
    if (auto handsv = json.FindMember("hands");
        handsv != json.MemberEnd() && handsv->value.IsArray()) {
      const auto& handsArray = handsv->value.GetArray();
      for (const auto& handJson : handsArray) {
        Hand hand;
        if (auto idv = handJson.FindMember("id");
            idv != handJson.MemberEnd() && idv->value.IsInt())
          hand.id = idv->value.GetInt();
        if (auto typev = handJson.FindMember("type");
            typev != handJson.MemberEnd() && typev->value.IsString())
          hand.type = std::strcmp(typev->value.GetString(), "left") == 0
                          ? HandType::Left
                          : HandType::Right;
        hands[hand.id] = hand;
      }
    }
    if (auto pointablesv = json.FindMember("pointables");
        pointablesv != json.MemberEnd() && pointablesv->value.IsArray()) {
      const auto& pointables = pointablesv->value.GetArray();
      for (auto& pointableJson : pointables) {
        if (auto handIdv = pointableJson.FindMember("handId");
            handIdv != pointableJson.MemberEnd() && handIdv->value.IsInt() &&
            hands.find(handIdv->value.GetInt()) != hands.end()) {
          Finger finger;
          Bone bone;

#define BONES_TO_FINGER(prev, next, boneType)               \
  bone.prevJoint = Vector(pointableJson[#prev].GetArray()); \
  bone.nextJoint = Vector(pointableJson[#next].GetArray()); \
  finger[boneType] = bone;

          BONES_TO_FINGER(carpPosition, mcpPosition, BoneType::Metacarpal)
          BONES_TO_FINGER(mcpPosition, pipPosition, BoneType::Proximal)
          BONES_TO_FINGER(pipPosition, dipPosition, BoneType::Medial)
          BONES_TO_FINGER(dipPosition, btipPosition, BoneType::Distal)

#undef BONES_TO_FINGER

          FingerType fingerType =
              static_cast<FingerType>(pointableJson["type"].GetInt());

          hands[handIdv->value.GetInt()].fingers[fingerType] = finger;
        }
      }
    }
  }

  QString toCsvLine(bool header = false) const {
    QString res;
    if (header) {
      res.append("id,");
      for (auto& hand : hands) {
        for (auto& finger : hand.second.fingers) {
          for (auto& bone : finger.second) {
            QString prefix = FingerTypeName(finger.first) +
                             QString(BoneTypeName(bone.first));
            res.append(prefix + "DirectionX,");
            res.append(prefix + "DirectionY,");
            res.append(prefix + "DirectionZ,");
          }
        }
        break;
      }
    } else {
      res.append(QString::number(id) + ",");
      for (auto& hand : hands) {
        for (auto& finger : hand.second.fingers) {
          for (auto& bone : finger.second) {
            auto d = bone.second.direction();
            res.append(QString::number(d.x) + ",");
            res.append(QString::number(d.y) + ",");
            res.append(QString::number(d.z) + ",");
          }
        }
        break;
      }
    }
    return res.append('\n');
  }
};

}  // namespace mxt
