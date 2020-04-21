#pragma once

#include "Base.h"

// Hospital is a place where we put V8 objects which have attached some external data which need some cleanup

// Our strategy is to hold v8::Global of the JS object, but make it weak and when we receive a callback about object
// going away we do the needed cleanup work.
// To make this flexible we accept cleanup function as a parameter when accepting patient into the hospital.
//
// Please note that we keep one hospital per isolate. So when isolate is about to go away we make sure the hospital
// is emptied before. The reasons why we have to do that are described in more detail in Tracer.h.

using V8Patient = v8::Global<v8::Object>;
using PatientClenupFn = void(v8::Local<v8::Object>);

struct HospitalRecord {
  V8Patient m_v8_patient;
  PatientClenupFn* m_cleanup_fn;

  HospitalRecord(v8::Local<v8::Object> v8_patient, PatientClenupFn* cleanup_fn) : m_cleanup_fn(cleanup_fn) {
    m_v8_patient.Reset(v8_patient->GetIsolate(), v8_patient);
  }
};

typedef std::unordered_set<HospitalRecord*> HospitalRecords;

void hospitalizePatient(v8::Local<v8::Object> v8_patient, PatientClenupFn* cleanup_fn);

class CHospital {
  v8::IsolateRef m_v8_isolate;
  HospitalRecords m_records;

 public:
  explicit CHospital(v8::IsolateRef v8_isolate);
  ~CHospital();

  void AcceptPatient(v8::Local<v8::Object> v8_patient, PatientClenupFn* cleanup_fn);
  void PatientIsAboutToDie(v8::IsolateRef v8_isolate, HospitalRecord* record);

 protected:
  void KillPatient(HospitalRecord* record);
};