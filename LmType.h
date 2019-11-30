// by cildhdi

#pragma once

#include <qjsonarray.h>
#include <qjsondocument.h>
#include <qjsonobject.h>
#include <map>

#define LM_CASE(name) \
  case name:          \
    return #name;     \
    break;

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

  Vector direction() const {
    return Vector({(nextJoint.x - prevJoint.x), (nextJoint.y - prevJoint.y),
                   (nextJoint.z - prevJoint.z)});
  }
};

enum FingerType { Thumb, Index, Middle, Ring, Pinky };

struct Finger {
  std::map<BoneType, Bone> bones;
  Vector tipPosition;
};

enum class HandType { Left, Right };
struct Hand {
  int id = 0;
  HandType type = HandType::Left;
  std::map<FingerType, Finger> fingers;
  Vector palmPosition, palmNormal;
};

namespace detail {

inline const char* boneTypeName(BoneType type) {
  switch (type) {
    LM_CASE(Metacarpal)
    LM_CASE(Proximal) LM_CASE(Medial) LM_CASE(Distal) default : return 0;
    break;
  }
}

inline const char* fingerTypeName(FingerType type) {
  switch (type) {
    LM_CASE(Thumb)
    LM_CASE(Index)
    LM_CASE(Middle) LM_CASE(Ring) LM_CASE(Pinky) default : return 0;
    break;
  }
}

inline QString makeXyzString(const QString& vectorName) {
  QString result{};
  result.append(vectorName + "X,");
  result.append(vectorName + "Y,");
  result.append(vectorName + "Z,");
  return result;
}

inline QString makeXyzValue(const Vector& vector) {
  QString result{};
  result.append(QString::number(vector.x) + ",");
  result.append(QString::number(vector.y) + ",");
  result.append(QString::number(vector.z) + ",");
  return result;
}

}  // namespace detail

struct Frame {
  long long id = 0;
  long long timestamp = 0;
  std::map<int, Hand> hands;

  auto findHandByType(HandType type) {
    auto it = hands.begin();
    while (it != hands.end() && it->second.type != type) {
      ++it;
    }
    return it;
  }

  Frame(QString const& text) {
    auto doc = QJsonDocument::fromJson(text.toLatin1());
    auto json = doc.object();
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
          Hand hand{};
          if (handObject.contains("id")) {
            hand.id = handObject.value("id").toInt();
          }
          if (handObject.contains("type")) {
            hand.type = handObject.value("type").toString() == "left"
                            ? HandType::Left
                            : HandType::Right;
          }
          if (handObject.contains("palmPosition")) {
            hand.palmPosition =
                Vector{handObject.value("palmPosition").toArray()};
          }
          if (handObject.contains("palmNormal")) {
            hand.palmNormal = Vector{handObject.value("palmNormal").toArray()};
          }
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
          Finger finger{};
          Bone bone{};

#define BONES_TO_FINGER(prev, next, boneType)                      \
  bone.prevJoint = Vector{pointableObject.value(#prev).toArray()}; \
  bone.nextJoint = Vector{pointableObject.value(#next).toArray()}; \
  finger.bones[boneType] = bone;

          BONES_TO_FINGER(carpPosition, mcpPosition, BoneType::Metacarpal)
          BONES_TO_FINGER(mcpPosition, pipPosition, BoneType::Proximal)
          BONES_TO_FINGER(pipPosition, dipPosition, BoneType::Medial)
          BONES_TO_FINGER(dipPosition, btipPosition, BoneType::Distal)

#undef BONES_TO_FINGER

          finger.tipPosition =
              Vector{pointableObject.value("tipPosition").toArray()};
          FingerType fingerType =
              static_cast<FingerType>(pointableObject.value("type").toInt());

          hands[handId].fingers[fingerType] = finger;
        }
      }
    }
  }

  QString toCsvLine(bool header = false) const {
    QString res;
    if (header) {
      res.append("id,");
      for (auto& hand : hands) {
        res.append(detail::makeXyzString("PalmPosition"));
        res.append(detail::makeXyzString("PalmNormal"));
        for (auto& finger : hand.second.fingers) {
          res.append(detail::makeXyzString(
              detail::fingerTypeName(finger.first) + QString{"TipPosition"}));
          for (auto& bone : finger.second.bones) {
            QString prefix = detail::fingerTypeName(finger.first) +
                             QString{detail::boneTypeName(bone.first)};
            res.append(detail::makeXyzString(prefix + "Direction"));
          }
        }
        break;
      }
    } else {
      res.append(QString::number(id) + ",");
      for (auto& hand : hands) {
        res.append(detail::makeXyzValue(hand.second.palmPosition));
        res.append(detail::makeXyzValue(hand.second.palmNormal));
        for (auto& finger : hand.second.fingers) {
          res.append(detail::makeXyzValue(finger.second.tipPosition));
          for (auto& bone : finger.second.bones) {
            res.append(detail::makeXyzValue(bone.second.direction()));
          }
        }
        break;
      }
    }
    return res.append('\n');
  }
};

}  // namespace mxt
