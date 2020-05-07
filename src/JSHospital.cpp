#include "JSHospital.h"
#include "JSIsolate.h"
#include "Logging.h"
#include "Printing.h"

#define TRACE(...) \
  LOGGER_INDENT;   \
  SPDLOG_LOGGER_TRACE(getLogger(kJSHospitalLogger), __VA_ARGS__)

void hospitalizePatient(v8::Local<v8::Object> v8_patient, PatientClenupFn cleanup_fn) {
  TRACE("hospitalizePatient v8_patient={}", v8_patient);
  auto v8_isolate = v8_patient->GetIsolate();
  auto isolate = CJSIsolate::FromV8(v8_isolate);
  isolate->Hospital().AcceptPatient(v8_patient, cleanup_fn);
}

static void v8WeakCallback(const v8::WeakCallbackInfo<HospitalRecord>& data) {
  auto v8_isolate = data.GetIsolate();
  auto isolate = CJSIsolate::FromV8(v8_isolate);
  auto hospital_record = data.GetParameter();
  TRACE("v8WeakCallback data.GetParameter={} v8_isolate={}", (void*)hospital_record, P$(v8_isolate));
  isolate->Hospital().PatientIsAboutToDie(v8_isolate, hospital_record);
}

// --------------------------------------------------------------------------------------------------------------------

CJSHospital::CJSHospital(v8x::ProtectedIsolatePtr v8_isolate) : m_v8_isolate(v8_isolate) {
  TRACE("CJSHospital::CJSHospital {} v8_isolate={}", THIS, m_v8_isolate);
}

CJSHospital::~CJSHospital() {
  TRACE("CJSHospital::~CJSHospital {}", THIS);
  // let all patients die
  auto it = m_records.begin();
  while (it != m_records.end()) {
    UnplugPatient(*it);
    it++;
  }
}

void CJSHospital::AcceptPatient(v8::Local<v8::Object> v8_patient, PatientClenupFn cleanup_fn) {
  TRACE("CJSHospital::AcceptPatient {} v8_patient={}", THIS, v8_patient);
  auto record = new HospitalRecord(v8_patient, cleanup_fn);
  m_records.insert(record);
  assert(!record->m_v8_patient.IsWeak());
  record->m_v8_patient.SetWeak(record, &v8WeakCallback, v8::WeakCallbackType::kFinalizer);
}

void CJSHospital::PatientIsAboutToDie(v8::Isolate* v8_isolate, HospitalRecord* record) {
  TRACE("CJSHospital::PatientIsAboutToDie {} record={} ", THIS, (void*)record);

  // just a sanity check that the weak callback is related to our isolate
  assert(v8_isolate == m_v8_isolate.giveMeRawIsolateAndTrustMe());
  assert(m_records.find(record) != m_records.end());

  // remove our records
  m_records.erase(record);

  UnplugPatient(record);
}

void CJSHospital::UnplugPatient(HospitalRecord* record) {
  TRACE("CJSHospital::UnplugPatient {} record={} ", THIS, (void*)record);

  // call custom cleanup function
  auto v8_isolate = m_v8_isolate.lock();
  record->m_cleanup_fn(record->m_v8_patient.Get(v8_isolate));

  // unplug him
  record->m_v8_patient.Reset();

  delete record;
}
