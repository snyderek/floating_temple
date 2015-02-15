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
#include <string>
#include <tr1/unordered_map>
#include <vector>

#include "base/const_shared_ptr.h"
#include "base/integral_types.h"
#include "base/linked_ptr.h"
#include "base/macros.h"
#include "base/mutex.h"
#include "include/c++/local_object.h"
#include "include/c++/value.h"

namespace floating_temple {

class DeserializationContext;
class PeerObject;
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

class LocalObjectImpl : public LocalObject {
 public:
  virtual std::size_t Serialize(void* buffer, std::size_t buffer_size,
                                SerializationContext* context) const;

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

  virtual LocalObject* Clone() const;
  virtual void InvokeMethod(Thread* thread,
                            PeerObject* peer_object,
                            const std::string& method_name,
                            const std::vector<Value>& parameters,
                            Value* return_value);
  virtual std::string Dump() const;

 protected:
  virtual void PopulateObjectProto(ObjectProto* object_proto,
                                   SerializationContext* context) const;

 private:
  DISALLOW_COPY_AND_ASSIGN(NoneObject);
};

class BoolObject : public LocalObjectImpl {
 public:
  explicit BoolObject(bool b);

  virtual LocalObject* Clone() const;
  virtual void InvokeMethod(Thread* thread,
                            PeerObject* peer_object,
                            const std::string& method_name,
                            const std::vector<Value>& parameters,
                            Value* return_value);
  virtual std::string Dump() const;

  static BoolObject* ParseBoolProto(const BoolProto& bool_proto);

 protected:
  virtual void PopulateObjectProto(ObjectProto* object_proto,
                                   SerializationContext* context) const;

 private:
  const bool b_;

  DISALLOW_COPY_AND_ASSIGN(BoolObject);
};

class IntObject : public LocalObjectImpl {
 public:
  explicit IntObject(int64 n);

  virtual LocalObject* Clone() const;
  virtual void InvokeMethod(Thread* thread,
                            PeerObject* peer_object,
                            const std::string& method_name,
                            const std::vector<Value>& parameters,
                            Value* return_value);
  virtual std::string Dump() const;

  static IntObject* ParseIntProto(const IntProto& int_proto);

 protected:
  virtual void PopulateObjectProto(ObjectProto* object_proto,
                                   SerializationContext* context) const;

 private:
  const int64 n_;

  DISALLOW_COPY_AND_ASSIGN(IntObject);
};

class StringObject : public LocalObjectImpl {
 public:
  explicit StringObject(const std::string& s);

  virtual LocalObject* Clone() const;
  virtual void InvokeMethod(Thread* thread,
                            PeerObject* peer_object,
                            const std::string& method_name,
                            const std::vector<Value>& parameters,
                            Value* return_value);
  virtual std::string Dump() const;

  static StringObject* ParseStringProto(const StringProto& string_proto);

 protected:
  virtual void PopulateObjectProto(ObjectProto* object_proto,
                                   SerializationContext* context) const;

 private:
  const std::string s_;

  DISALLOW_COPY_AND_ASSIGN(StringObject);
};

class SymbolTableObject : public LocalObjectImpl {
 public:
  SymbolTableObject();

  virtual LocalObject* Clone() const;
  virtual void InvokeMethod(Thread* thread,
                            PeerObject* peer_object,
                            const std::string& method_name,
                            const std::vector<Value>& parameters,
                            Value* return_value);
  virtual std::string Dump() const;

  static SymbolTableObject* ParseSymbolTableProto(
      const SymbolTableProto& symbol_table_proto,
      DeserializationContext* context);

 protected:
  virtual void PopulateObjectProto(ObjectProto* object_proto,
                                   SerializationContext* context) const;

 private:
  typedef std::vector<linked_ptr<std::tr1::unordered_map<std::string,
                                                         PeerObject*> > >
      ScopeVector;

  std::string GetStringForLogging() const;

  ScopeVector scopes_;
  // TODO(dss): Is this mutex necessary?
  mutable Mutex scopes_mu_;

  DISALLOW_COPY_AND_ASSIGN(SymbolTableObject);
};

class ExpressionObject : public LocalObjectImpl {
 public:
  explicit ExpressionObject(const const_shared_ptr<Expression>& expression);

  virtual LocalObject* Clone() const;
  virtual void InvokeMethod(Thread* thread,
                            PeerObject* peer_object,
                            const std::string& method_name,
                            const std::vector<Value>& parameters,
                            Value* return_value);
  virtual std::string Dump() const;

  static ExpressionObject* ParseExpressionProto(
      const ExpressionProto& expression_proto);

 protected:
  virtual void PopulateObjectProto(ObjectProto* object_proto,
                                   SerializationContext* context) const;

 private:
  const const_shared_ptr<Expression> expression_;

  DISALLOW_COPY_AND_ASSIGN(ExpressionObject);
};

class ListObject : public LocalObjectImpl {
 public:
  explicit ListObject(const std::vector<PeerObject*>& items);

  virtual LocalObject* Clone() const;
  virtual void InvokeMethod(Thread* thread,
                            PeerObject* peer_object,
                            const std::string& method_name,
                            const std::vector<Value>& parameters,
                            Value* return_value);
  virtual std::string Dump() const;

  static ListObject* ParseListProto(const ListProto& list_proto,
                                    DeserializationContext* context);

 protected:
  virtual void PopulateObjectProto(ObjectProto* object_proto,
                                   SerializationContext* context) const;

 private:
  std::vector<PeerObject*> items_;
  mutable Mutex items_mu_;

  DISALLOW_COPY_AND_ASSIGN(ListObject);
};

class MapObject : public LocalObjectImpl {
 public:
  MapObject();

  virtual LocalObject* Clone() const;
  virtual void InvokeMethod(Thread* thread,
                            PeerObject* peer_object,
                            const std::string& method_name,
                            const std::vector<Value>& parameters,
                            Value* return_value);
  virtual std::string Dump() const;

  static MapObject* ParseMapProto(const MapProto& map_proto,
                                  DeserializationContext* context);

 protected:
  virtual void PopulateObjectProto(ObjectProto* object_proto,
                                   SerializationContext* context) const;

 private:
  std::tr1::unordered_map<std::string, PeerObject*> map_;

  DISALLOW_COPY_AND_ASSIGN(MapObject);
};

class RangeIteratorObject : public LocalObjectImpl {
 public:
  RangeIteratorObject(int64 limit, int64 start);

  virtual LocalObject* Clone() const;
  virtual void InvokeMethod(Thread* thread,
                            PeerObject* peer_object,
                            const std::string& method_name,
                            const std::vector<Value>& parameters,
                            Value* return_value);
  virtual std::string Dump() const;

  static RangeIteratorObject* ParseRangeIteratorProto(
      const RangeIteratorProto& range_iterator_proto);

 protected:
  virtual void PopulateObjectProto(ObjectProto* object_proto,
                                   SerializationContext* context) const;

 private:
  const int64 limit_;
  int64 i_;
  mutable Mutex i_mu_;

  DISALLOW_COPY_AND_ASSIGN(RangeIteratorObject);
};

class Function : public LocalObjectImpl {
 public:
  virtual void InvokeMethod(Thread* thread,
                            PeerObject* peer_object,
                            const std::string& method_name,
                            const std::vector<Value>& parameters,
                            Value* return_value);

 protected:
  virtual PeerObject* Call(
      PeerObject* symbol_table_object, Thread* thread,
      const std::vector<PeerObject*>& parameters) const = 0;
};

class ListFunction : public Function {
 public:
  ListFunction();

  virtual LocalObject* Clone() const;
  virtual std::string Dump() const;

 protected:
  virtual void PopulateObjectProto(ObjectProto* object_proto,
                                   SerializationContext* context) const;
  virtual PeerObject* Call(PeerObject* symbol_table_object, Thread* thread,
                           const std::vector<PeerObject*>& parameters) const;

 private:
  DISALLOW_COPY_AND_ASSIGN(ListFunction);
};

class SetVariableFunction : public Function {
 public:
  SetVariableFunction();

  virtual LocalObject* Clone() const;
  virtual std::string Dump() const;

 protected:
  virtual void PopulateObjectProto(ObjectProto* object_proto,
                                   SerializationContext* context) const;
  virtual PeerObject* Call(PeerObject* symbol_table_object, Thread* thread,
                           const std::vector<PeerObject*>& parameters) const;

 private:
  DISALLOW_COPY_AND_ASSIGN(SetVariableFunction);
};

class ForFunction : public Function {
 public:
  ForFunction();

  virtual LocalObject* Clone() const;
  virtual std::string Dump() const;

 protected:
  virtual void PopulateObjectProto(ObjectProto* object_proto,
                                   SerializationContext* context) const;
  virtual PeerObject* Call(PeerObject* symbol_table_object, Thread* thread,
                           const std::vector<PeerObject*>& parameters) const;

 private:
  DISALLOW_COPY_AND_ASSIGN(ForFunction);
};

class RangeFunction : public Function {
 public:
  RangeFunction();

  virtual LocalObject* Clone() const;
  virtual std::string Dump() const;

 protected:
  virtual void PopulateObjectProto(ObjectProto* object_proto,
                                   SerializationContext* context) const;
  virtual PeerObject* Call(PeerObject* symbol_table_object, Thread* thread,
                           const std::vector<PeerObject*>& parameters) const;

 private:
  DISALLOW_COPY_AND_ASSIGN(RangeFunction);
};

class PrintFunction : public Function {
 public:
  PrintFunction();

  virtual LocalObject* Clone() const;
  virtual std::string Dump() const;

 protected:
  virtual void PopulateObjectProto(ObjectProto* object_proto,
                                   SerializationContext* context) const;
  virtual PeerObject* Call(PeerObject* symbol_table_object, Thread* thread,
                           const std::vector<PeerObject*>& parameters) const;

 private:
  DISALLOW_COPY_AND_ASSIGN(PrintFunction);
};

class AddFunction : public Function {
 public:
  AddFunction();

  virtual LocalObject* Clone() const;
  virtual std::string Dump() const;

 protected:
  virtual void PopulateObjectProto(ObjectProto* object_proto,
                                   SerializationContext* context) const;
  virtual PeerObject* Call(PeerObject* symbol_table_object, Thread* thread,
                           const std::vector<PeerObject*>& parameters) const;

 private:
  DISALLOW_COPY_AND_ASSIGN(AddFunction);
};

class BeginTranFunction : public Function {
 public:
  BeginTranFunction();

  virtual LocalObject* Clone() const;
  virtual std::string Dump() const;

 protected:
  virtual void PopulateObjectProto(ObjectProto* object_proto,
                                   SerializationContext* context) const;
  virtual PeerObject* Call(PeerObject* symbol_table_object, Thread* thread,
                           const std::vector<PeerObject*>& parameters) const;

 private:
  DISALLOW_COPY_AND_ASSIGN(BeginTranFunction);
};

class EndTranFunction : public Function {
 public:
  EndTranFunction();

  virtual LocalObject* Clone() const;
  virtual std::string Dump() const;

 protected:
  virtual void PopulateObjectProto(ObjectProto* object_proto,
                                   SerializationContext* context) const;
  virtual PeerObject* Call(PeerObject* symbol_table_object, Thread* thread,
                           const std::vector<PeerObject*>& parameters) const;

 private:
  DISALLOW_COPY_AND_ASSIGN(EndTranFunction);
};

class IfFunction : public Function {
 public:
  IfFunction();

  virtual LocalObject* Clone() const;
  virtual std::string Dump() const;

 protected:
  virtual void PopulateObjectProto(ObjectProto* object_proto,
                                   SerializationContext* context) const;
  virtual PeerObject* Call(PeerObject* symbol_table_object, Thread* thread,
                           const std::vector<PeerObject*>& parameters) const;

 private:
  DISALLOW_COPY_AND_ASSIGN(IfFunction);
};

class NotFunction : public Function {
 public:
  NotFunction();

  virtual LocalObject* Clone() const;
  virtual std::string Dump() const;

 protected:
  virtual void PopulateObjectProto(ObjectProto* object_proto,
                                   SerializationContext* context) const;
  virtual PeerObject* Call(PeerObject* symbol_table_object, Thread* thread,
                           const std::vector<PeerObject*>& parameters) const;

 private:
  DISALLOW_COPY_AND_ASSIGN(NotFunction);
};

class IsSetFunction : public Function {
 public:
  IsSetFunction();

  virtual LocalObject* Clone() const;
  virtual std::string Dump() const;

 protected:
  virtual void PopulateObjectProto(ObjectProto* object_proto,
                                   SerializationContext* context) const;
  virtual PeerObject* Call(PeerObject* symbol_table_object, Thread* thread,
                           const std::vector<PeerObject*>& parameters) const;

 private:
  DISALLOW_COPY_AND_ASSIGN(IsSetFunction);
};

class WhileFunction : public Function {
 public:
  WhileFunction();

  virtual LocalObject* Clone() const;
  virtual std::string Dump() const;

 protected:
  virtual void PopulateObjectProto(ObjectProto* object_proto,
                                   SerializationContext* context) const;
  virtual PeerObject* Call(PeerObject* symbol_table_object, Thread* thread,
                           const std::vector<PeerObject*>& parameters) const;

 private:
  DISALLOW_COPY_AND_ASSIGN(WhileFunction);
};

class LessThanFunction : public Function {
 public:
  LessThanFunction();

  virtual LocalObject* Clone() const;
  virtual std::string Dump() const;

 protected:
  virtual void PopulateObjectProto(ObjectProto* object_proto,
                                   SerializationContext* context) const;
  virtual PeerObject* Call(PeerObject* symbol_table_object, Thread* thread,
                           const std::vector<PeerObject*>& parameters) const;

 private:
  DISALLOW_COPY_AND_ASSIGN(LessThanFunction);
};

class LenFunction : public Function {
 public:
  LenFunction();

  virtual LocalObject* Clone() const;
  virtual std::string Dump() const;

 protected:
  virtual void PopulateObjectProto(ObjectProto* object_proto,
                                   SerializationContext* context) const;
  virtual PeerObject* Call(PeerObject* symbol_table_object, Thread* thread,
                           const std::vector<PeerObject*>& parameters) const;

 private:
  DISALLOW_COPY_AND_ASSIGN(LenFunction);
};

class AppendFunction : public Function {
 public:
  AppendFunction();

  virtual LocalObject* Clone() const;
  virtual std::string Dump() const;

 protected:
  virtual void PopulateObjectProto(ObjectProto* object_proto,
                                   SerializationContext* context) const;
  virtual PeerObject* Call(PeerObject* symbol_table_object, Thread* thread,
                           const std::vector<PeerObject*>& parameters) const;

 private:
  DISALLOW_COPY_AND_ASSIGN(AppendFunction);
};

class GetAtFunction : public Function {
 public:
  GetAtFunction();

  virtual LocalObject* Clone() const;
  virtual std::string Dump() const;

 protected:
  virtual void PopulateObjectProto(ObjectProto* object_proto,
                                   SerializationContext* context) const;
  virtual PeerObject* Call(PeerObject* symbol_table_object, Thread* thread,
                           const std::vector<PeerObject*>& parameters) const;

 private:
  DISALLOW_COPY_AND_ASSIGN(GetAtFunction);
};

class MapIsSetFunction : public Function {
 public:
  MapIsSetFunction();

  virtual LocalObject* Clone() const;
  virtual std::string Dump() const;

 protected:
  virtual void PopulateObjectProto(ObjectProto* object_proto,
                                   SerializationContext* context) const;
  virtual PeerObject* Call(PeerObject* symbol_table_object, Thread* thread,
                           const std::vector<PeerObject*>& parameters) const;

 private:
  DISALLOW_COPY_AND_ASSIGN(MapIsSetFunction);
};

class MapGetFunction : public Function {
 public:
  MapGetFunction();

  virtual LocalObject* Clone() const;
  virtual std::string Dump() const;

 protected:
  virtual void PopulateObjectProto(ObjectProto* object_proto,
                                   SerializationContext* context) const;
  virtual PeerObject* Call(PeerObject* symbol_table_object, Thread* thread,
                           const std::vector<PeerObject*>& parameters) const;

 private:
  DISALLOW_COPY_AND_ASSIGN(MapGetFunction);
};

class MapSetFunction : public Function {
 public:
  MapSetFunction();

  virtual LocalObject* Clone() const;
  virtual std::string Dump() const;

 protected:
  virtual void PopulateObjectProto(ObjectProto* object_proto,
                                   SerializationContext* context) const;
  virtual PeerObject* Call(PeerObject* symbol_table_object, Thread* thread,
                           const std::vector<PeerObject*>& parameters) const;

 private:
  DISALLOW_COPY_AND_ASSIGN(MapSetFunction);
};

}  // namespace toy_lang
}  // namespace floating_temple

#endif  // TOY_LANG_LOCAL_OBJECT_IMPL_H_
