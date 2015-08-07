// Floating Temple
// Copyright 2015 Derek S. Snyder
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef TOY_LANG_LOCAL_OBJECT_IMPL_H_
#define TOY_LANG_LOCAL_OBJECT_IMPL_H_

#include <cstddef>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "base/integral_types.h"
#include "base/linked_ptr.h"
#include "base/macros.h"
#include "base/mutex.h"
#include "include/c++/value.h"
#include "include/c++/versioned_local_object.h"

namespace floating_temple {

class DeserializationContext;
class ObjectReference;
class SerializationContext;
class Thread;

namespace toy_lang {

class BoolProto;
class Expression;
class ExpressionProto;
class IntProto;
class ListProto;
class MapProto;
class ObjectProto;
class RangeIteratorProto;
class StringProto;
class SymbolTableProto;

class LocalObjectImpl : public VersionedLocalObject {
 public:
  std::size_t Serialize(void* buffer, std::size_t buffer_size,
                        SerializationContext* context) const override;

  static LocalObjectImpl* Deserialize(const void* buffer,
                                      std::size_t buffer_size,
                                      DeserializationContext* context);

 protected:
  virtual void PopulateObjectProto(ObjectProto* object_proto,
                                   SerializationContext* context) const = 0;
};

class NoneObject : public LocalObjectImpl {
 public:
  NoneObject();

  VersionedLocalObject* Clone() const override;
  void InvokeMethod(Thread* thread,
                    ObjectReference* object_reference,
                    const std::string& method_name,
                    const std::vector<Value>& parameters,
                    Value* return_value) override;
  void Dump(DumpContext* dc) const override;

 protected:
  void PopulateObjectProto(ObjectProto* object_proto,
                           SerializationContext* context) const override;

 private:
  DISALLOW_COPY_AND_ASSIGN(NoneObject);
};

class BoolObject : public LocalObjectImpl {
 public:
  explicit BoolObject(bool b);

  VersionedLocalObject* Clone() const override;
  void InvokeMethod(Thread* thread,
                    ObjectReference* object_reference,
                    const std::string& method_name,
                    const std::vector<Value>& parameters,
                    Value* return_value) override;
  void Dump(DumpContext* dc) const override;

  static BoolObject* ParseBoolProto(const BoolProto& bool_proto);

 protected:
  void PopulateObjectProto(ObjectProto* object_proto,
                           SerializationContext* context) const override;

 private:
  const bool b_;

  DISALLOW_COPY_AND_ASSIGN(BoolObject);
};

class IntObject : public LocalObjectImpl {
 public:
  explicit IntObject(int64 n);

  VersionedLocalObject* Clone() const override;
  void InvokeMethod(Thread* thread,
                    ObjectReference* object_reference,
                    const std::string& method_name,
                    const std::vector<Value>& parameters,
                    Value* return_value) override;
  void Dump(DumpContext* dc) const override;

  static IntObject* ParseIntProto(const IntProto& int_proto);

 protected:
  void PopulateObjectProto(ObjectProto* object_proto,
                           SerializationContext* context) const override;

 private:
  const int64 n_;

  DISALLOW_COPY_AND_ASSIGN(IntObject);
};

class StringObject : public LocalObjectImpl {
 public:
  explicit StringObject(const std::string& s);

  VersionedLocalObject* Clone() const override;
  void InvokeMethod(Thread* thread,
                    ObjectReference* object_reference,
                    const std::string& method_name,
                    const std::vector<Value>& parameters,
                    Value* return_value) override;
  void Dump(DumpContext* dc) const override;

  static StringObject* ParseStringProto(const StringProto& string_proto);

 protected:
  void PopulateObjectProto(ObjectProto* object_proto,
                           SerializationContext* context) const override;

 private:
  const std::string s_;

  DISALLOW_COPY_AND_ASSIGN(StringObject);
};

class SymbolTableObject : public LocalObjectImpl {
 public:
  SymbolTableObject();

  VersionedLocalObject* Clone() const override;
  void InvokeMethod(Thread* thread,
                    ObjectReference* object_reference,
                    const std::string& method_name,
                    const std::vector<Value>& parameters,
                    Value* return_value) override;
  void Dump(DumpContext* dc) const override;

  static SymbolTableObject* ParseSymbolTableProto(
      const SymbolTableProto& symbol_table_proto,
      DeserializationContext* context);

 protected:
  void PopulateObjectProto(ObjectProto* object_proto,
                           SerializationContext* context) const override;

 private:
  typedef std::vector<linked_ptr<std::unordered_map<std::string,
                                                    ObjectReference*>>>
      ScopeVector;

  std::string GetStringForLogging() const;

  ScopeVector scopes_;
  // TODO(dss): Is this mutex necessary?
  mutable Mutex scopes_mu_;

  DISALLOW_COPY_AND_ASSIGN(SymbolTableObject);
};

class ExpressionObject : public LocalObjectImpl {
 public:
  explicit ExpressionObject(
      const std::shared_ptr<const Expression>& expression);

  VersionedLocalObject* Clone() const override;
  void InvokeMethod(Thread* thread,
                    ObjectReference* object_reference,
                    const std::string& method_name,
                    const std::vector<Value>& parameters,
                    Value* return_value) override;
  void Dump(DumpContext* dc) const override;

  static ExpressionObject* ParseExpressionProto(
      const ExpressionProto& expression_proto);

 protected:
  void PopulateObjectProto(ObjectProto* object_proto,
                           SerializationContext* context) const override;

 private:
  const std::shared_ptr<const Expression> expression_;

  DISALLOW_COPY_AND_ASSIGN(ExpressionObject);
};

class ListObject : public LocalObjectImpl {
 public:
  explicit ListObject(const std::vector<ObjectReference*>& items);

  VersionedLocalObject* Clone() const override;
  void InvokeMethod(Thread* thread,
                    ObjectReference* object_reference,
                    const std::string& method_name,
                    const std::vector<Value>& parameters,
                    Value* return_value) override;
  void Dump(DumpContext* dc) const override;

  static ListObject* ParseListProto(const ListProto& list_proto,
                                    DeserializationContext* context);

 protected:
  void PopulateObjectProto(ObjectProto* object_proto,
                           SerializationContext* context) const override;

 private:
  std::vector<ObjectReference*> items_;
  mutable Mutex items_mu_;

  DISALLOW_COPY_AND_ASSIGN(ListObject);
};

class MapObject : public LocalObjectImpl {
 public:
  MapObject();

  VersionedLocalObject* Clone() const override;
  void InvokeMethod(Thread* thread,
                    ObjectReference* object_reference,
                    const std::string& method_name,
                    const std::vector<Value>& parameters,
                    Value* return_value) override;
  void Dump(DumpContext* dc) const override;

  static MapObject* ParseMapProto(const MapProto& map_proto,
                                  DeserializationContext* context);

 protected:
  void PopulateObjectProto(ObjectProto* object_proto,
                           SerializationContext* context) const override;

 private:
  // TODO(dss): Should this map be protected by a mutex?
  std::unordered_map<std::string, ObjectReference*> map_;

  DISALLOW_COPY_AND_ASSIGN(MapObject);
};

class RangeIteratorObject : public LocalObjectImpl {
 public:
  RangeIteratorObject(int64 limit, int64 start);

  VersionedLocalObject* Clone() const override;
  void InvokeMethod(Thread* thread,
                    ObjectReference* object_reference,
                    const std::string& method_name,
                    const std::vector<Value>& parameters,
                    Value* return_value) override;
  void Dump(DumpContext* dc) const override;

  static RangeIteratorObject* ParseRangeIteratorProto(
      const RangeIteratorProto& range_iterator_proto);

 protected:
  void PopulateObjectProto(ObjectProto* object_proto,
                           SerializationContext* context) const override;

 private:
  const int64 limit_;
  int64 i_;
  mutable Mutex i_mu_;

  DISALLOW_COPY_AND_ASSIGN(RangeIteratorObject);
};

class Function : public LocalObjectImpl {
 public:
  void InvokeMethod(Thread* thread,
                    ObjectReference* object_reference,
                    const std::string& method_name,
                    const std::vector<Value>& parameters,
                    Value* return_value) override;

 protected:
  virtual ObjectReference* Call(
      ObjectReference* symbol_table_object, Thread* thread,
      const std::vector<ObjectReference*>& parameters) const = 0;
};

class ListFunction : public Function {
 public:
  ListFunction();

  VersionedLocalObject* Clone() const override;
  void Dump(DumpContext* dc) const override;

 protected:
  void PopulateObjectProto(ObjectProto* object_proto,
                           SerializationContext* context) const override;
  ObjectReference* Call(
      ObjectReference* symbol_table_object, Thread* thread,
      const std::vector<ObjectReference*>& parameters) const override;

 private:
  DISALLOW_COPY_AND_ASSIGN(ListFunction);
};

class SetVariableFunction : public Function {
 public:
  SetVariableFunction();

  VersionedLocalObject* Clone() const override;
  void Dump(DumpContext* dc) const override;

 protected:
  void PopulateObjectProto(ObjectProto* object_proto,
                           SerializationContext* context) const override;
  ObjectReference* Call(
      ObjectReference* symbol_table_object, Thread* thread,
      const std::vector<ObjectReference*>& parameters) const override;

 private:
  DISALLOW_COPY_AND_ASSIGN(SetVariableFunction);
};

class ForFunction : public Function {
 public:
  ForFunction();

  VersionedLocalObject* Clone() const override;
  void Dump(DumpContext* dc) const override;

 protected:
  void PopulateObjectProto(ObjectProto* object_proto,
                           SerializationContext* context) const override;
  ObjectReference* Call(
      ObjectReference* symbol_table_object, Thread* thread,
      const std::vector<ObjectReference*>& parameters) const override;

 private:
  DISALLOW_COPY_AND_ASSIGN(ForFunction);
};

class RangeFunction : public Function {
 public:
  RangeFunction();

  VersionedLocalObject* Clone() const override;
  void Dump(DumpContext* dc) const override;

 protected:
  void PopulateObjectProto(ObjectProto* object_proto,
                           SerializationContext* context) const override;
  ObjectReference* Call(
      ObjectReference* symbol_table_object, Thread* thread,
      const std::vector<ObjectReference*>& parameters) const override;

 private:
  DISALLOW_COPY_AND_ASSIGN(RangeFunction);
};

class PrintFunction : public Function {
 public:
  PrintFunction();

  VersionedLocalObject* Clone() const override;
  void Dump(DumpContext* dc) const override;

 protected:
  void PopulateObjectProto(ObjectProto* object_proto,
                           SerializationContext* context) const override;
  ObjectReference* Call(
      ObjectReference* symbol_table_object, Thread* thread,
      const std::vector<ObjectReference*>& parameters) const override;

 private:
  DISALLOW_COPY_AND_ASSIGN(PrintFunction);
};

class AddFunction : public Function {
 public:
  AddFunction();

  VersionedLocalObject* Clone() const override;
  void Dump(DumpContext* dc) const override;

 protected:
  void PopulateObjectProto(ObjectProto* object_proto,
                           SerializationContext* context) const override;
  ObjectReference* Call(
      ObjectReference* symbol_table_object, Thread* thread,
      const std::vector<ObjectReference*>& parameters) const override;

 private:
  DISALLOW_COPY_AND_ASSIGN(AddFunction);
};

class BeginTranFunction : public Function {
 public:
  BeginTranFunction();

  VersionedLocalObject* Clone() const override;
  void Dump(DumpContext* dc) const override;

 protected:
  void PopulateObjectProto(ObjectProto* object_proto,
                           SerializationContext* context) const override;
  ObjectReference* Call(
      ObjectReference* symbol_table_object, Thread* thread,
      const std::vector<ObjectReference*>& parameters) const override;

 private:
  DISALLOW_COPY_AND_ASSIGN(BeginTranFunction);
};

class EndTranFunction : public Function {
 public:
  EndTranFunction();

  VersionedLocalObject* Clone() const override;
  void Dump(DumpContext* dc) const override;

 protected:
  void PopulateObjectProto(ObjectProto* object_proto,
                           SerializationContext* context) const override;
  ObjectReference* Call(
      ObjectReference* symbol_table_object, Thread* thread,
      const std::vector<ObjectReference*>& parameters) const override;

 private:
  DISALLOW_COPY_AND_ASSIGN(EndTranFunction);
};

class IfFunction : public Function {
 public:
  IfFunction();

  VersionedLocalObject* Clone() const override;
  void Dump(DumpContext* dc) const override;

 protected:
  void PopulateObjectProto(ObjectProto* object_proto,
                           SerializationContext* context) const override;
  ObjectReference* Call(
      ObjectReference* symbol_table_object, Thread* thread,
      const std::vector<ObjectReference*>& parameters) const override;

 private:
  DISALLOW_COPY_AND_ASSIGN(IfFunction);
};

class NotFunction : public Function {
 public:
  NotFunction();

  VersionedLocalObject* Clone() const override;
  void Dump(DumpContext* dc) const override;

 protected:
  void PopulateObjectProto(ObjectProto* object_proto,
                           SerializationContext* context) const override;
  ObjectReference* Call(
      ObjectReference* symbol_table_object, Thread* thread,
      const std::vector<ObjectReference*>& parameters) const override;

 private:
  DISALLOW_COPY_AND_ASSIGN(NotFunction);
};

class IsSetFunction : public Function {
 public:
  IsSetFunction();

  VersionedLocalObject* Clone() const override;
  void Dump(DumpContext* dc) const override;

 protected:
  void PopulateObjectProto(ObjectProto* object_proto,
                           SerializationContext* context) const override;
  ObjectReference* Call(
      ObjectReference* symbol_table_object, Thread* thread,
      const std::vector<ObjectReference*>& parameters) const override;

 private:
  DISALLOW_COPY_AND_ASSIGN(IsSetFunction);
};

class WhileFunction : public Function {
 public:
  WhileFunction();

  VersionedLocalObject* Clone() const override;
  void Dump(DumpContext* dc) const override;

 protected:
  void PopulateObjectProto(ObjectProto* object_proto,
                           SerializationContext* context) const override;
  ObjectReference* Call(
      ObjectReference* symbol_table_object, Thread* thread,
      const std::vector<ObjectReference*>& parameters) const override;

 private:
  DISALLOW_COPY_AND_ASSIGN(WhileFunction);
};

class LessThanFunction : public Function {
 public:
  LessThanFunction();

  VersionedLocalObject* Clone() const override;
  void Dump(DumpContext* dc) const override;

 protected:
  void PopulateObjectProto(ObjectProto* object_proto,
                           SerializationContext* context) const override;
  ObjectReference* Call(
      ObjectReference* symbol_table_object, Thread* thread,
      const std::vector<ObjectReference*>& parameters) const override;

 private:
  DISALLOW_COPY_AND_ASSIGN(LessThanFunction);
};

class LenFunction : public Function {
 public:
  LenFunction();

  VersionedLocalObject* Clone() const override;
  void Dump(DumpContext* dc) const override;

 protected:
  void PopulateObjectProto(ObjectProto* object_proto,
                           SerializationContext* context) const override;
  ObjectReference* Call(
      ObjectReference* symbol_table_object, Thread* thread,
      const std::vector<ObjectReference*>& parameters) const override;

 private:
  DISALLOW_COPY_AND_ASSIGN(LenFunction);
};

class AppendFunction : public Function {
 public:
  AppendFunction();

  VersionedLocalObject* Clone() const override;
  void Dump(DumpContext* dc) const override;

 protected:
  void PopulateObjectProto(ObjectProto* object_proto,
                           SerializationContext* context) const override;
  ObjectReference* Call(
      ObjectReference* symbol_table_object, Thread* thread,
      const std::vector<ObjectReference*>& parameters) const override;

 private:
  DISALLOW_COPY_AND_ASSIGN(AppendFunction);
};

class GetAtFunction : public Function {
 public:
  GetAtFunction();

  VersionedLocalObject* Clone() const override;
  void Dump(DumpContext* dc) const override;

 protected:
  void PopulateObjectProto(ObjectProto* object_proto,
                           SerializationContext* context) const override;
  ObjectReference* Call(
      ObjectReference* symbol_table_object, Thread* thread,
      const std::vector<ObjectReference*>& parameters) const override;

 private:
  DISALLOW_COPY_AND_ASSIGN(GetAtFunction);
};

class MapIsSetFunction : public Function {
 public:
  MapIsSetFunction();

  VersionedLocalObject* Clone() const override;
  void Dump(DumpContext* dc) const override;

 protected:
  void PopulateObjectProto(ObjectProto* object_proto,
                           SerializationContext* context) const override;
  ObjectReference* Call(
      ObjectReference* symbol_table_object, Thread* thread,
      const std::vector<ObjectReference*>& parameters) const override;

 private:
  DISALLOW_COPY_AND_ASSIGN(MapIsSetFunction);
};

class MapGetFunction : public Function {
 public:
  MapGetFunction();

  VersionedLocalObject* Clone() const override;
  void Dump(DumpContext* dc) const override;

 protected:
  void PopulateObjectProto(ObjectProto* object_proto,
                           SerializationContext* context) const override;
  ObjectReference* Call(
      ObjectReference* symbol_table_object, Thread* thread,
      const std::vector<ObjectReference*>& parameters) const override;

 private:
  DISALLOW_COPY_AND_ASSIGN(MapGetFunction);
};

class MapSetFunction : public Function {
 public:
  MapSetFunction();

  VersionedLocalObject* Clone() const override;
  void Dump(DumpContext* dc) const override;

 protected:
  void PopulateObjectProto(ObjectProto* object_proto,
                           SerializationContext* context) const override;
  ObjectReference* Call(
      ObjectReference* symbol_table_object, Thread* thread,
      const std::vector<ObjectReference*>& parameters) const override;

 private:
  DISALLOW_COPY_AND_ASSIGN(MapSetFunction);
};

}  // namespace toy_lang
}  // namespace floating_temple

#endif  // TOY_LANG_LOCAL_OBJECT_IMPL_H_
