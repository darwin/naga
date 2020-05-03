#ifndef NAGA_JSHOSPITAL_H_
#define NAGA_JSHOSPITAL_H_

#include "Base.h"

// Hospital is a place where we put V8 objects which have attached some external data which need some cleanup.

// Our strategy is to hold JS object via v8::Global, but make it weak and when we receive a callback about an object
// going away we do the requested cleanup work. To make this flexible we accept cleanup function as a parameter
// when accepting patient into the hospital.
//
// Please note that we keep one hospital per isolate. So when an isolate is about to go away we make sure all patients
// in the hospital get properly unplugged (we call their cleanup routines). The reasons why we have to do that are
// described in more detail in Tracer.h.

using V8Patient = v8::Global<v8::Object>;
using PatientClenupFn = std::function<void(v8::Local<v8::Object>)>; // we want to be able to accept any lambda

struct HospitalRecord {
  V8Patient m_v8_patient;
  PatientClenupFn m_cleanup_fn;

  HospitalRecord(v8::Local<v8::Object> v8_patient, PatientClenupFn cleanup_fn) : m_cleanup_fn(cleanup_fn) {
    m_v8_patient.Reset(v8_patient->GetIsolate(), v8_patient);
  }
};

using HospitalRecords = std::unordered_set<HospitalRecord*>;

void hospitalizePatient(v8::Local<v8::Object> v8_patient, PatientClenupFn cleanup_fn);

class CJSHospital {
  v8::IsolatePtr m_v8_isolate;
  HospitalRecords m_records;

 public:
  explicit CJSHospital(v8::IsolatePtr v8_isolate);
  ~CJSHospital();

  void AcceptPatient(v8::Local<v8::Object> v8_patient, PatientClenupFn cleanup_fn);
  void PatientIsAboutToDie(v8::IsolatePtr v8_isolate, HospitalRecord* record);

 protected:
  void UnplugPatient(HospitalRecord* record);
};

#endif